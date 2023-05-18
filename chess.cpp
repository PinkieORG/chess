#include "chess.hpp"
#include <cassert>

bool move::isDiagonal() { return (std::abs(file) == std::abs(rank)); }

bool move::isStraight() { return (file == 0 || rank == 0); }

void move::directionize() {
    if (file != 0) {
        file = file / std::abs(file);
    }
    if (rank != 0) {
        rank = rank / std::abs(rank);
    }
}

bool move::operator==(move other) { return (rank == other.rank && file == other.file); }

bool move::operator!=(move other) { return !(*this == other); }

move move::vert(int offset) { return {0, offset}; }

move move::horiz(int offset) { return {offset, 0}; }

move move::abs(move m) { return {std::abs(m.file), std::abs(m.rank)}; }

move position::operator-(position other) { return {file - other.file, rank - other.rank}; }

position position::operator+(move other) {
    position result = {file + other.file, rank + other.rank};
    if (result.rank < 1 || result.rank > 8 || result.file < 1 || result.file > 8) {
        return position();
    }
    return result;
}

std::vector<position> position::allPositions() {
    std::vector<position> output;
    for(int r = 1; r <= 8; ++r) {
        for (int f = 1; f <= 8; ++f) {
            output.emplace_back(position{f, r});
        }
    }
    return output;
}

bool position::operator==(position other) { return (rank == other.rank && file == other.file); }

bool position::operator!=(position other) { return !(*this == other); }

bool chess::canMove(struct position from, struct position to, enum piece_type type, player player) {
    move m = from - to;
    switch (type) {
        case piece_type::pawn:
            return canMovePawn(from, to, player);
        case piece_type::king:
            return canMoveKing(from, to, player);
        case piece_type::rook:
            return canMoveRook(m);
        case piece_type::knight:
            return canMoveKnight(m);
        case piece_type::bishop:
            return canMoveBishop(m);
        case piece_type::queen:
            return canMoveQueen(m);
        default:
            return false;
    }
}

bool chess::canMovePawn(struct position from, struct position to, player player) {
    move m = to - from;


    int p;
    player == player::white ? p = 1 : p = -1;
    // Standard move.
    if (m == move::vert(p)) {
        return true;
    }
    // Two step.
    if (!at(from).didMove && m == move::vert(2 * p)) {
        return true;
    }
    // Capture move.
    if (m == move{1, p} || m == move{-1, p}) {
        if (!at(to).is_empty) {
            return true;
        }
        occupant toLapse = at(to + move::vert(-p));
        // En passant.
        if (toLapse.owner != player && toLapse.didTwoStep) {
            return true;
        }
    }
    return false;
}

bool chess::canMoveKing(struct position from, struct position to, player player) {
    if (isCastling(from, to, player)) {
        return true;
    }
    move m = move::abs(to - from);
    return (m.file <= 1 && m.rank <= 1);
}

bool chess::canMoveRook(move m) {
    return m.isStraight();
}

bool chess::canMoveKnight(move m) {
    m = move::abs(m);
    return ((m.file == 1 && m.rank == 2) || (m.file == 2 && m.rank == 1));
}

bool chess::canMoveBishop(move m) {
    return m.isDiagonal();
}

bool chess::canMoveQueen(move m) {
    return (m.isStraight() || m.isDiagonal());
}

chess::chess() {

    std::vector<std::vector<occupant>> occupants(8, std::vector<occupant>(8));


    _occupants = occupants;
    _player = player::white;
    for (int i = 1; i <= 8; ++i) {
        placeOccupant(occupant {player::white, piece_type::pawn}, {i, 2});
        placeOccupant(occupant {player::black, piece_type::pawn}, {i, 7});
    }



    player colour = player::white;
    for (int i: {1, 8}) {
        placeOccupant(occupant{colour, piece_type::rook},   {1, i});
        placeOccupant(occupant{colour, piece_type::knight}, {2, i});
        placeOccupant(occupant{colour, piece_type::bishop}, {3, i});
        placeOccupant(occupant{colour, piece_type::queen},  {4, i});
        placeOccupant(occupant{colour, piece_type::king},   {5, i});
        placeOccupant(occupant{colour, piece_type::bishop}, {6, i});
        placeOccupant(occupant{colour, piece_type::knight}, {7, i});
        placeOccupant(occupant{colour, piece_type::rook},   {8, i});


        colour = player::black;
    }
}

occupant chess::at(position at) const {
    if (at == position()) {
        return occupant();
    }
    return _occupants[at.file - 1][at.rank - 1];
}

bool chess::isBlocked(position from, position to, player player) {

    if (!at(to).is_empty) {
        if (at(to).owner == player) {
            return true;
        }
        if (at(from).piece == piece_type::pawn && (to - from).isStraight() && at(to).owner == getOpponent()) {
            return true;
        }
    }
    move dir = (to - from);
    if (!dir.isDiagonal() && !dir.isStraight()) {
        return false;
    }
    dir.directionize();
    position pos = from + dir;
    while (pos != to) {
        if (!at(pos).is_empty) {
            return true;
        }
        pos = pos + dir;
    }
    if (isCastling(from, to, player)) {
        // In case of the queen-side castling we also need to check that the rook does pass above the vacant square.
        return !at(to + move::horiz(-1)).is_empty;
    }
    return false;
}

void chess::makeMove(position from, position to) {
    placeOccupant(getOccupant(from), to);
    placeOccupant(occupant(), from);
}

void chess::swapPlayer() {

    _player == player::white ? _player = player::black : _player = player::white;
}

bool chess::isChecked() {
    position king = findKingPosition();
    for (position p: position::allPositions()) {
        occupant o = getOccupant(p);
        if (!o.is_empty && o.owner != _player) {
            if (canMove(p, king, o.piece, getOpponent()) && !isBlocked(p, king, getOpponent())) {
                // Pawn cannot capture when the move is straight.
                if (at(p).piece == piece_type::pawn && (king - p).isStraight()) {
                    continue;
                }
                return true;
            }
        }
    }
    return false;
}

bool chess::wouldCheck(position from, position to) {
    bool result = false;
    occupant tmpTarget = at(to);
    if (isEnPassant(from, to)) {

        occupant tmpUp = at(to + move::vert(1));
        occupant tmpDown = at(to + move::vert(-1));
        applyEnPassant(to);


        makeMove(from, to);
        if (isChecked()) {
            result = true;
        }
        makeMove(to, from);
        placeOccupant(tmpUp, to +  move::vert(1));
        placeOccupant(tmpDown, to + move::vert(-1));
    } else {
        makeMove(from, to);
        if (isChecked()) {
            result = true;
        }
        makeMove(to, from);
    }
    placeOccupant(tmpTarget, to);
    return result;
}

result chess::play(position from, position to, piece_type promote /* = piece_type::pawn */) {



    restartLapses();
    bool wasChecked = isChecked();

    if (at(from).is_empty) {
        return result::no_piece;
    }
    occupant p = at(from);
    if (p.owner != _player) {
        return result::bad_piece;
    }
    if (!canMove(from, to, p.piece, _player)) {
        return result::bad_move;
    }
    if (isBlocked(from, to, _player)) {
        return result::blocked;
    }
    if (isLapsed(from, to)) {
        return result::lapsed;
    }
    if (isCastling(from, to, _player)) {
        if (wasChecked) {
            return result::in_check;
        }
        if (wouldCheckCastling(from, to)) {
            return result::would_check;
        }
        if (hasMoved(from, to)) {
            return result::has_moved;
        }
    } else if (wouldCheck(from, to)) {
        if (wasChecked) {
            return result::in_check;
        }
        return result::would_check;
    }
    if (isPromote(from, to)) {
        if (!isValidPromote(promote)) {
            return result::bad_promote;
        }
        applyPromote(from, promote);
    }


    result result = result::ok;
    if (!at(to).is_empty || isEnPassant(from, to)) {
        result = result::capture;
    }
    if (isEnPassant(from, to)) {
        applyEnPassant(to);
    }
    setFlags(from, to);
    if (isCastling(from, to, _player)) {
        makeCastling(from, to);
    } else {
        makeMove(from, to);
    }
    swapPlayer();
    return result;
 }

void chess::print() {
    for (int r = 8; r >= 1; --r) {
        std::cout << "-----------------\n";
        for (int f = 1; f <= 8; ++f) {
            std::cout << "|";
            if (!at({f, r}).is_empty) {
                occupant o = at({f, r});
                if (o.owner == player::white) {
                    switch (o.piece) {
                        case piece_type::pawn :
                            std::cout << "♙";
                            break;
                        case piece_type::rook :
                            std::cout << "♖";
                            break;
                        case piece_type::knight :
                            std::cout << "♘";
                            break;
                        case piece_type::bishop :
                            std::cout << "♗";
                            break;
                        case piece_type::king :
                            std::cout << "♔";
                            break;
                        case piece_type::queen :
                            std::cout << "♕";
                            break;
                    }
                } else {
                    switch (o.piece) {
                        case piece_type::pawn :
                            std::cout << "♟";
                            break;
                        case piece_type::rook :
                            std::cout << "♜";
                            break;
                        case piece_type::knight :
                            std::cout << "♞";
                            break;
                        case piece_type::bishop :
                            std::cout << "♝";
                            break;
                        case piece_type::king :
                            std::cout << "♚";
                            break;
                        case piece_type::queen :
                            std::cout << "♛";
                            break;
                    }
                }
            } else {
                std::cout << " ";
            }
        }
        std::cout << "|\n";
    }
    std::cout << "-----------------\n";
}

occupant &chess::getOccupant(position at) {
    return _occupants[at.file - 1][at.rank - 1];
}

void chess::placeOccupant(occupant occupant, position at) {
    _occupants[at.file - 1][at.rank - 1] = occupant;
}

void chess::setFlags(position from, position to) {
    if (at(from).piece == piece_type::pawn) {
        if (move::abs(to - from) == move::vert(2)) {
            getOccupant(from).didTwoStep = true;
            getOccupant(from).canBeLapsed = true;
        } else {
            getOccupant(from).didTwoStep = false;
        }
    }
    getOccupant(from).didMove = true;
}

position chess::findKingPosition() {
    for (position p: position::allPositions()) {
        occupant o = getOccupant(p);

        if (!o.is_empty && o.piece == piece_type::king && o.owner == _player) {
            return p;
        }
    }
    return position();
}

player chess::getOpponent() {
    return _player == player::white ? player::black : player::white;
}

void chess::restartLapses() {
    for(position p: position::allPositions()) {
        occupant& o = getOccupant(p);
        if (!o.is_empty && o.owner == _player && o.piece == piece_type::pawn) {
            o.canBeLapsed = false;
        }
    }
}

bool chess::isLapsed(position from, position to) {
    int p;
    _player == player::white ? p = -1 : p = 1;
    occupant toLapse = at(to + move::vert(p));
    if (at(from).piece == piece_type::pawn && toLapse.owner == getOpponent() && toLapse.didTwoStep) {
        return !toLapse.canBeLapsed;
    }
    return false;
}

bool chess::isEnPassant(position from, position to) {
    int p;
    _player == player::white ? p = -1 : p = 1;
    occupant toLapse = at(to + move::vert(p));
    return (at(from).piece == piece_type::pawn && toLapse.owner == getOpponent() && toLapse.canBeLapsed);
}

void chess::applyEnPassant(position at) {
    int p;
    _player == player::white ? p = -1 : p = 1;
    position toLapsePos = at + move::vert(p);
    placeOccupant(occupant(), toLapsePos);
}

bool chess::isCastling(position from, position to, player player) {
    move m = to - from;
    int p;
    player == player::white ? p = 1 : p = 8;

    return from == position{5, p} &&
           ((m == move::horiz(2) && at({8, p}).piece == piece_type::rook && at({8, p}).owner == player) ||
            (m == move::horiz(-2) && at({1, p}).piece == piece_type::rook && at({1, p}).owner == player));
}

bool chess::wouldCheckCastling(position from, position to) {
    move m = to - from;
    int p;
    m.file == 2 ? p = 1 : p = -1;
    return (wouldCheck(from, from + move::horiz(p)) || wouldCheck(from, from + move::horiz(2 * p)));
}

bool chess::hasMoved(position from, position to) {
    move m = to - from;
    if (at(from).didMove) {
        return true;
    }
    if (m.file == 2) {
        // King-side rook
        return (at(from + move::horiz(3)).didMove);
    }
    // Queen-side rook
    return (at(from + move::horiz(-4)).didMove);
}

bool chess::isPromote(position from, position to) {
    return  (at(from).piece == piece_type::pawn && (to.rank == 1 || to.rank == 8));
}

bool chess::isValidPromote(piece_type promote) {
    return (promote != piece_type::pawn && promote != piece_type::king);
}

void chess::applyPromote(position at, piece_type promote) {
    //placeOccupant(occupant{_player, promote}, at);
    getOccupant(at).piece = promote;
}

void chess::makeCastling(position from, position to) {
    move m = to - from;
    // Move the rook.
    if (m.file == 2) {
        makeMove(to + move::horiz(1), to + move::horiz(-1));
    } else {
        makeMove(to + move::horiz(-2), to + move::horiz(1));
    }
    placeOccupant(getOccupant(from), to);
    placeOccupant(occupant(), from);
}

/* ##### TESTS ############################################################################## */

void test_enpassant() {
    chess my_chess = chess();
    my_chess.play({4, 2}, {4, 4});
    my_chess.play({1, 7}, {1, 6});
    my_chess.play({4, 4}, {4, 5});
    my_chess.print();
    assert (!my_chess.getOccupant({4, 5}).canBeLapsed);
    my_chess.play({3, 7}, {3, 5});
    my_chess.print();
    assert (my_chess.getOccupant({3, 5}).canBeLapsed);
    assert (my_chess.play({4, 5}, {3, 6}) == result::capture);
    my_chess.print();
}

void test_castling() {
    chess my_chess = chess();
    // big white
    my_chess.placeOccupant(occupant(),{2, 1});
    my_chess.placeOccupant(occupant(),{3, 1});
    my_chess.placeOccupant(occupant(),{4, 1});
    my_chess.print();
    my_chess.play({5, 1}, {3, 1});
    my_chess.print();
    // small black
    my_chess.placeOccupant(occupant(),{6, 8});
    my_chess.placeOccupant(occupant(),{7, 8});
    my_chess.print();
    my_chess.play({5, 8}, {7, 8});
    my_chess.print();
}

void test_promote() {
    chess my_chess = chess();
    my_chess.play({2, 2},{2, 4});
    my_chess.play({6, 7},{6, 5});
    my_chess.play({2, 4},{2, 5});
    my_chess.play({6, 5},{6, 4});
    my_chess.print();
    my_chess.play({3, 2},{3, 4});
    my_chess.print();
    my_chess.play({6, 4},{6, 3});
    my_chess.print();
    my_chess.play({3, 4},{3, 5});
    my_chess.print();
    my_chess.play({2, 7},{2, 6});
    my_chess.print();
    my_chess.play({3, 5},{2, 6});
    my_chess.print();
    my_chess.play({2, 8},{3, 6});
    my_chess.print();
    my_chess.play({2, 6},{1, 7});
    my_chess.print();
    my_chess.play({1, 8},{2, 8});
    my_chess.print();
    my_chess.play({1, 7},{2, 8}, piece_type::bishop);
    my_chess.print();
    my_chess.play({6, 3}, {5, 2});
    my_chess.print();
    assert( my_chess.play({2, 8}, {3, 7}) == result::capture);
    my_chess.print();

}

int main()
{
    chess my_chess = chess();
    my_chess.print();
    position a7 = {1, 7};
    position a6 = {1, 6};
    assert( my_chess.play( a7, a6 ) == result::bad_piece);
    test_enpassant();
    test_castling();
    test_promote();

    chess c = chess();
    assert(c.play( {1, 2}, {1, 4} ) == result::ok);
    assert(c.play( {1, 7}, {1, 5} ) == result::ok);
    assert(c.play( {1, 4}, {1, 5} ) == result::blocked);

    assert(c.play( {7, 2}, {7, 3} ) == result::ok);
    assert(c.play( {7, 7}, {7, 6} ) == result::ok);
    assert(c.play( {6, 1}, {8, 3} ) == result::ok);
    assert(c.play( {7, 8}, {6, 6} ) == result::ok);
    assert(c.play( {7, 1}, {6, 3} ) == result::ok);
    assert(c.play( {6, 8}, {8, 6} ) == result::ok);
    c.print();
    assert(c.play( {8, 1}, {7, 1} ) == result::ok);
    c.print();
    assert(c.play( {5, 7}, {5, 6} ) == result::ok);
    c.print();
    assert(c.play( {7, 1}, {8, 1} ) == result::ok);
    c.print();
    assert(c.play( {6, 6}, {8, 5} ) == result::ok);
    c.print();
    assert(c.play( {5, 1}, {7, 1} ) == result::has_moved);

}
