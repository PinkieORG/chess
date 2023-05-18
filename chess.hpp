
#include <vector>
#include <iostream>


// Difference of two positions. Behaves similarly to a vector in affine space.
struct move {
    int file;
    int rank;

    bool isDiagonal();

    bool isStraight();

    void directionize();

    bool operator==(move other);

    bool operator!=(move other);

    static move abs(move move);

    static move vert(int offset);

    static move horiz(int offset);
};

// Positions on the chess board. Behaves similarly to a point in affine space.
struct position {
    int file = 0;
    int rank = 0;

    // Returns all possible positions on the board.

    static std::vector<position> allPositions();

    bool operator==(position other);

    bool operator!=(position other);

    move operator-(position other);

    // If result falls out of the board returns {0, 0}.
    position operator+(move other);
};

enum class piece_type { pawn, rook, knight, bishop, queen, king };

enum class player { white, black };

/* The following are the possible outcomes of ‹play›. The outcomes
 * are shown in the order of precedence, i.e. the first applicable
 * is returned.
 *
 * ├┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┼┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┤
 * │ ‹capture›     │ the move was legal and resulted in a capture │
 * │ ‹ok›          │ the move was legal and was performed         │
 * │ ‹no_piece›    │ there is no piece on the ‹from› square       │
 * │ ‹bad_piece›   │ the piece on ‹from› is not ours              │
 * │ ‹bad_move›    │ this move is not available for this piece    │
 * │ ‹blocked›     │ another piece is in the way                  │
 * │ ‹lapsed›      │ «en passant» capture is no longer allowed    │
 * │ ‹in_check›    │ the player is currently in check and the     │
 * │               │ move does not get them out of it             │
 * │ ‹would_check› │ the move would place the player in check     │
 * │ ‹has_moved›   │ one of the castling pieces has already moved │
 * │ ‹bad_promote› │ promotion to a pawn or king was attempted    │
 *
 * Attempting an «en passant» when the pieces are in the wrong place
 * is a ‹bad_move›. In addition to ‹has_moved›, (otherwise legal)
 * castling may give:
 *
 *  • ‹blocked› – some pieces are in the way,
 *  • ‹in_check› – the king is currently in check,
 *  • ‹would_check› – would pass through or end up in check. */

enum class result {
    capture, ok, no_piece, bad_piece, bad_move, blocked, lapsed, in_check,
    would_check, has_moved, bad_promote
};

struct occupant {

    bool is_empty {true};
    player owner {player::white};
    piece_type piece {piece_type::pawn};
    bool didMove {false};
    // Is true when the pawn moved two squares from its initial position and didn't moved since.


    bool didTwoStep {false};


    bool canBeLapsed {false};

    occupant() = default;;

    occupant(player player, piece_type piece)
        :   is_empty(false),
            owner(player),
            piece(piece) {}

};

class chess {

    player _player;

    std::vector<std::vector<occupant>> _occupants;
public:

    chess();

    bool canMove(struct position from, struct position to, enum piece_type type, player player);


    bool canMovePawn(struct position from, struct position to, player player);

    bool canMoveKing(struct position from, struct position to, player player);

    static bool canMoveRook(move m);

    static bool canMoveKnight(move m);

    static bool canMoveBishop(move m);

    static bool canMoveQueen(move m);

    void swapPlayer();

    bool isChecked();

    // Checks only straight moves, diagonal moves or castling.
    bool isBlocked(position from, position to, player player);

    void makeMove(position from, position to);

    occupant& getOccupant(position at);

    void placeOccupant(occupant occupant, position at);

    // Sets flags of just moved occupant.
    void setFlags(position from, position to);

    // Returns position of the king of the current player.
    position findKingPosition();

    player getOpponent();

    bool wouldCheck(position from, position to);

    // Sets canBeLapsed to false for all pawns of the current player.
    void restartLapses();

    // Returns true if en passant is no longer possible.
    bool isLapsed(position from, position to);

    void applyEnPassant(position at);

    bool isEnPassant(position from, position to);

    bool isCastling(position from, position to, player player);

    // Checks if king would pass through a check.
    bool wouldCheckCastling(position from, position to);

    void makeCastling(position from, position to);

    // Checks if castling pieces have moved.
    bool hasMoved(position from, position to);

    bool isPromote(position from, position to);

    static bool isValidPromote(piece_type promote);

    void applyPromote(position at, piece_type promote);

    result play(position from, position to, piece_type promote = piece_type::pawn);

    // For position {0, 0} returns new default occupant.
    occupant at(position) const;

    // Prints the board using text.
    void print();
};
