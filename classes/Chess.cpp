#include "Chess.h"
#include "MagicBitboards.h"
#include <limits>
#include <cmath>

#define WHITE 1
#define BLACK 2

Chess::Chess()
{
    _grid = new Grid(8, 8);

    initMagicBitboards();

    for(int i=0; i<64; i++) {
        _knightBitboards[i] = generateKnightMoveBitboard(i);
        _kingBitboards[i] = generateKingMoveBitboard(i);
    }
    
    for(int i=0; i<128; i++) { _bitboardLookup[i] = 0; }
    _bitboardLookup['P'] = WHITE_PAWNS;
    _bitboardLookup['N'] = WHITE_KNIGHTS;
    _bitboardLookup['B'] = WHITE_BISHOPS;
    _bitboardLookup['R'] = WHITE_ROOKS;
    _bitboardLookup['Q'] = WHITE_QUEENS;
    _bitboardLookup['K'] = WHITE_KING;
    _bitboardLookup['p'] = BLACK_PAWNS;
    _bitboardLookup['n'] = BLACK_KNIGHTS;
    _bitboardLookup['b'] = BLACK_BISHOPS;
    _bitboardLookup['r'] = BLACK_ROOKS;
    _bitboardLookup['q'] = BLACK_QUEENS;
    _bitboardLookup['k'] = BLACK_KING;
    _bitboardLookup['0'] = EMPTY_SQUARES;
}

Chess::~Chess()
{
    cleanupMagicBitboards();
    delete _grid;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()-128];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
    // FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/8/RNBQKBNR");

    _currentPlayer = WHITE;
    _moves = generateAllMoves();
    startGame();
}

Bitboard Chess::generateKnightMoveBitboard(int square) {
    std::pair<int, int> knightOffsets[] = {
        { 2, 1 }, { 2, -1 }, { -2, 1 }, { -2, -1 },
        { 1, 2 }, { 1, -2 }, { -1, 2 }, { -1, -2 }
    };
    Bitboard bitboard = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    char knightPiece = _currentPlayer == WHITE ? 'N' : 'n';
    constexpr uint64_t oneBit = 1;
    for (auto [dr, df] : knightOffsets) {
        int r = rank + dr, f = file + df;
        if (r >= 0 && r < 8 && f >= 0 && f < 8) {
            bitboard |= oneBit << (r * 8 + f);
        }
    }
    return bitboard;
}

Bitboard Chess::generateKingMoveBitboard(int square) {
    std::pair<int, int> kingOffsets[] = {
        { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 }, 
        { 1, 1 }, { 1, -1 }, { -1, 1 }, { -1, -1 }
    };

    Bitboard bitboard = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    constexpr uint64_t oneBit = 1;
    for (auto [dr, df] : kingOffsets) {
        int r = rank + dr, f = file + df;
        if (r >= 0 && r < 8 && f >= 0 && f < 8) {
            bitboard |= oneBit << (r * 8 + f);
        }
    }
    return bitboard;
}

std::vector<BitMove> Chess::generateAllMoves() {
    std::vector<BitMove> moves;
    moves.reserve(32);
    std::string state = stateString();

    for (int i = 0; i < e_numBitboards; i++) {
        _bitboards[i] = 0;
    }

    for (int i = 0; i < 64; i++) {
        int bitIndex = _bitboardLookup[state[i]];
        _bitboards[bitIndex] |= 1ULL << i;
        if (state[i] != '0') {
            _bitboards[OCCUPANCY] |= 1ULL << i;
            _bitboards[isupper(state[i]) ? WHITE_ALL_PIECES : BLACK_ALL_PIECES] |= 1ULL << i;
        }
    }

    int bitIndex = _currentPlayer == WHITE ? WHITE_PAWNS : BLACK_PAWNS;
    int oppBitIndex = _currentPlayer == WHITE ? BLACK_PAWNS : WHITE_PAWNS;
    int oppAllPieces = _currentPlayer == WHITE ? BLACK_ALL_PIECES : WHITE_ALL_PIECES;

    generateKnightMoves(moves, _bitboards[WHITE_KNIGHTS + bitIndex], _bitboards[WHITE_ALL_PIECES + bitIndex].getData());
    generateKingMoves(moves, _bitboards[WHITE_KING + bitIndex], _bitboards[WHITE_ALL_PIECES + bitIndex].getData());
    generateBishopMoves(moves, _bitboards[WHITE_BISHOPS + bitIndex], _bitboards[OCCUPANCY].getData(), _bitboards[WHITE_ALL_PIECES + bitIndex].getData());
    generateRookMoves(moves, _bitboards[WHITE_ROOKS + bitIndex], _bitboards[OCCUPANCY].getData(), _bitboards[WHITE_ALL_PIECES + bitIndex].getData());
    generatePawnMoves(moves, _bitboards[WHITE_PAWNS + bitIndex], ~_bitboards[OCCUPANCY].getData(), _bitboards[oppAllPieces], _currentPlayer);

    return moves;
}

void Chess::generateAllKnightMoves(std::vector<BitMove>& moves, std::string& state) {
    // All possible L-shapes from the position
    std::pair<int, int> knightOffsets[] = {
        { 2, 1 }, { 2, -1 }, { -2, 1 }, { -2, -1 },
        { 1, 2 }, { 1, -2 }, { -1, 2 }, { -1, -2 }
    };

    int index = 0;

    char knightPiece = _currentPlayer == WHITE ? 'N' : 'n';
    for (char square : state) {
        if (square == knightPiece) {
            int rank = index / 8;
            int file = index % 8;
            constexpr uint64_t oneBit = 1;
            for (auto [dr, df] : knightOffsets) {
                int r = rank + dr, f = file + df;
                if (r >= 0 && r < 8 && f >= 0 && f < 8) {
                    moves.emplace_back(index, r*8+f, Knight);
                }
            }
        }
        index++;
    }
}

void Chess::generateKnightMoves(std::vector<BitMove>& moves, Bitboard piecesBitboard, uint64_t friendlies) {
    piecesBitboard.forEachBit([&] (int fromSquare) {
        Bitboard canMoveTo = _knightBitboards[fromSquare].getData() & ~friendlies;
        canMoveTo.forEachBit([&] (int toSquare) {
            moves.emplace_back(fromSquare, toSquare, Knight);
        });
    });
}

void Chess::generateKingMoves(std::vector<BitMove>& moves, Bitboard piecesBitboard, uint64_t friendlies) {
    piecesBitboard.forEachBit([&] (int fromSquare) {
        Bitboard canMoveTo = _kingBitboards[fromSquare].getData() & ~friendlies;
        canMoveTo.forEachBit([&] (int toSquare) {
            moves.emplace_back(fromSquare, toSquare, King);
        });
    });
}

void Chess::generateBishopMoves(std::vector<BitMove>& moves, Bitboard piecesBitboard, uint64_t occupancy, uint64_t friendlies) {
    piecesBitboard.forEachBit([&] (int fromSquare) {
        Bitboard canMoveTo = Bitboard(getBishopAttacks(fromSquare, occupancy) & ~friendlies); 
        canMoveTo.forEachBit([&] (int toSquare) {
            moves.emplace_back(fromSquare, toSquare, Bishop);
        });
    });
}

void Chess::generateRookMoves(std::vector<BitMove>& moves, Bitboard piecesBitboard, uint64_t occupancy, uint64_t friendlies) {
    piecesBitboard.forEachBit([&] (int fromSquare) {
        Bitboard canMoveTo = Bitboard(getRookAttacks(fromSquare, occupancy) & ~friendlies); 
        canMoveTo.forEachBit([&] (int toSquare) {
            moves.emplace_back(fromSquare, toSquare, Rook);
        });
    });
}

void Chess::addPawnBitboardMovesToList(std::vector<BitMove>& moves, const Bitboard bitboard, const int shift) {
    if (bitboard.getData() == 0)
        return;

    bitboard.forEachBit([&](int toSquare) {
        int fromSquare = toSquare - shift; 
        moves.emplace_back(fromSquare, toSquare, Pawn);
    });
}

void Chess::generatePawnMoves(std::vector<BitMove>& moves, const Bitboard pawns, const Bitboard emptySquares, const Bitboard enemyPieces, char color) {
    if (pawns.getData() == 0)
        return;

    constexpr uint64_t NotAFile(0xFEFEFEFEFEFEFEFEULL); 
    constexpr uint64_t NotHFile(0x7F7F7F7F7F7F7F7FULL); 
    constexpr uint64_t Rank3(0x0000000000FF0000ULL);   
    constexpr uint64_t Rank6(0x0000FF0000000000ULL);  

    Bitboard singleMoves = (color == WHITE) ? (pawns.getData() << 8) & emptySquares.getData() : (pawns.getData() >> 8) & emptySquares.getData();
    Bitboard doubleMoves = (color == WHITE) ? ((singleMoves.getData() & Rank3) << 8) & emptySquares.getData() : ((singleMoves.getData() & Rank6) >> 8) & emptySquares.getData();
    Bitboard capturesLeft = (color == WHITE) ? ((pawns.getData() & NotAFile) << 7) & enemyPieces.getData() : ((pawns.getData() & NotAFile) >> 9) & enemyPieces.getData();
    Bitboard capturesRight = (color == WHITE) ? ((pawns.getData() & NotHFile) << 9) & enemyPieces.getData() : ((pawns.getData() & NotHFile) >> 7) & enemyPieces.getData();

    int shiftForward = (color == WHITE) ? 8 : -8;
    int doubleShift = (color == WHITE) ? 16 : -16;
    int captureLeftShift = (color == WHITE) ? 7 : -9;
    int captureRightShift = (color == WHITE) ? 9 : -7;

    addPawnBitboardMovesToList(moves, singleMoves, shiftForward);
    addPawnBitboardMovesToList(moves, doubleMoves, doubleShift);
    addPawnBitboardMovesToList(moves, capturesLeft, captureLeftShift);
    addPawnBitboardMovesToList(moves, capturesRight, captureRightShift);
}

void Chess::FENtoBoard(const std::string& fen) {
    // convert a FEN string to a board
    // FEN is a space delimited string with 6 fields
    // 1: piece placement (from white's perspective)
    // NOT PART OF THIS ASSIGNMENT BUT OTHER THINGS THAT CAN BE IN A FEN STRING
    // ARE BELOW
    // 2: active color (W or B)
    // 3: castling availability (KQkq or -)
    // 4: en passant target square (in algebraic notation, or -)
    // 5: halfmove clock (number of halfmoves since the last capture or pawn advance)
    int row = 7;
    int col = 0;

    for (auto c : fen) {
        if (c == ' ') 
            break;
        if (c == '/') {
            row--;
            col = 0;
            continue;
        }
        if (isdigit(c)) {
            col += (c - '0'); 
            continue;
        }

        ChessPiece piece;
        int playerNum = isupper(c) ? 0 : 1;
        char lowerC = tolower(c);

        switch (lowerC) {
            case 'p': piece = Pawn;   break;
            case 'n': piece = Knight; break;
            case 'b': piece = Bishop; break;
            case 'r': piece = Rook;   break;
            case 'q': piece = Queen;  break;
            case 'k': piece = King;   break;
            default: continue; 
        }

        Bit* b = PieceForPlayer(playerNum, piece);
        ChessSquare* s = _grid->getSquare(col, row);
        b->setPosition(s->getPosition());
        b->setGameTag(playerNum == 0 ? piece : piece + 128);
        s->setBit(b);

        col++;
    }
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    // need to implement friendly/unfriendly in bit so for now this hack
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor != currentPlayer) return false;

    ChessSquare* square = (ChessSquare*) &src;

    int squareIndex = square->getSquareIndex();
    for (auto move : _moves) {
        if (move.from == squareIndex) {
            return true;
        }
    }

    return false;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare* squareSrc = (ChessSquare*) &src;
    ChessSquare* squareDst = (ChessSquare*) &dst;
    int squareIndexSrc = squareSrc->getSquareIndex();
    int squareIndexDst = squareDst->getSquareIndex();

    for (auto move : _moves) {
        if (move.from == squareIndexSrc && move.to == squareIndexDst) {
            return true;
        }
    }
    return false;
}

void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) {
    _currentPlayer = (_currentPlayer == WHITE ? BLACK : WHITE);
    _moves = generateAllMoves();
    clearBoardHighlights();
    endTurn();
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    return nullptr;
}

bool Chess::checkForDraw()
{
    return false;
}

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation( x, y );
        }
    );
    return s;
}

void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });
}
