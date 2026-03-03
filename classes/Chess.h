#pragma once

#include "Game.h"
#include "Grid.h"
#include "Bitboard.h"

constexpr int pieceSize = 80;

enum AllBitBoards
{
    WHITE_PAWNS,
    WHITE_KNIGHTS,
    WHITE_BISHOPS,
    WHITE_ROOKS,
    WHITE_QUEENS,
    WHITE_KING,
    BLACK_PAWNS,
    BLACK_KNIGHTS,
    BLACK_BISHOPS,
    BLACK_ROOKS,
    BLACK_QUEENS,
    BLACK_KING,
    WHITE_ALL_PIECES,
    BLACK_ALL_PIECES,
    OCCUPANCY,
    EMPTY_SQUARES,
    e_numBitboards
};

class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;
	void bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;

    void stopGame() override;

    Player *checkForWinner() override;
    bool checkForDraw() override;

    std::vector<BitMove> generateAllMoves();
    void generateAllKnightMoves(std::vector<BitMove>& moves, std::string& state);
    void generateKnightMoves(std::vector<BitMove>& moves, Bitboard knightBitboard, uint64_t friendlies);
    void generateKingMoves(std::vector<BitMove>& moves, Bitboard knightBitboard, uint64_t friendlies);
    void generateBishopMoves(std::vector<BitMove>& moves, Bitboard knightBitboard, uint64_t occupancy, uint64_t friendlies);
    void generateRookMoves(std::vector<BitMove>& moves, Bitboard knightBitboard, uint64_t occupancy, uint64_t friendlies);
    void generatePawnMoves(std::vector<BitMove>& moves, const Bitboard pawns, const Bitboard emptySquares, const Bitboard enemyPieces, char color);
    void addPawnBitboardMovesToList(std::vector<BitMove>& moves, const Bitboard bitboard, const int shift);
    Bitboard generateKnightMoveBitboard(int square);
    Bitboard generateKingMoveBitboard(int square);

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    Grid* getGrid() override { return _grid; }

private:
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;

    int _currentPlayer;
    Grid* _grid;
    std::vector<BitMove> _moves;
    Bitboard _bitboards[e_numBitboards];
    Bitboard _knightBitboards[64];
    Bitboard _kingBitboards[64];
    int _bitboardLookup[128];
};