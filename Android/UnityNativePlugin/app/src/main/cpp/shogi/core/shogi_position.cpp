/*
  Apery, a USI shogi playing engine derived from Stockfish, a UCI chess playing engine.
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2016 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad
  Copyright (C) 2011-2017 Hiraoka Takuya

  Apery is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Apery is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "shogi_position.hpp"
#include "shogi_move.hpp"
#include "shogi_mt64bit.hpp"
#include "shogi_generateMoves.hpp"
#include "shogi_tt.hpp"
#include "shogi_search.hpp"
#include "shogi_evaluate.hpp"

namespace Shogi
{
    Key Position::zobrist_[PieceTypeNum][SquareNum][ColorNum];
    Key Position::zobHand_[HandPieceNum][ColorNum];
    Key Position::zobExclusion_;
    
    const HuffmanCode HuffmanCodedPos::boardCodeTable[PieceNone] = {
        {Binary<         0>::value, 1}, // Empty
        {Binary<         1>::value, 4}, // BPawn
        {Binary<        11>::value, 6}, // BLance
        {Binary<       111>::value, 6}, // BKnight
        {Binary<      1011>::value, 6}, // BSilver
        {Binary<     11111>::value, 8}, // BBishop
        {Binary<    111111>::value, 8}, // BRook
        {Binary<      1111>::value, 6}, // BGold
        {Binary<         0>::value, 0}, // BKing 玉の位置は別途、位置を符号化する。使用しないので numOfBit を 0 にしておく。
        {Binary<      1001>::value, 4}, // BProPawn
        {Binary<    100011>::value, 6}, // BProLance
        {Binary<    100111>::value, 6}, // BProKnight
        {Binary<    101011>::value, 6}, // BProSilver
        {Binary<  10011111>::value, 8}, // BHorse
        {Binary<  10111111>::value, 8}, // BDragona
        {Binary<         0>::value, 0}, // 使用しないので numOfBit を 0 にしておく。
        {Binary<         0>::value, 0}, // 使用しないので numOfBit を 0 にしておく。
        {Binary<       101>::value, 4}, // WPawn
        {Binary<     10011>::value, 6}, // WLance
        {Binary<     10111>::value, 6}, // WKnight
        {Binary<     11011>::value, 6}, // WSilver
        {Binary<   1011111>::value, 8}, // WBishop
        {Binary<   1111111>::value, 8}, // WRook
        {Binary<    101111>::value, 6}, // WGold
        {Binary<         0>::value, 0}, // WKing 玉の位置は別途、位置を符号化する。
        {Binary<      1101>::value, 4}, // WProPawn
        {Binary<    110011>::value, 6}, // WProLance
        {Binary<    110111>::value, 6}, // WProKnight
        {Binary<    111011>::value, 6}, // WProSilver
        {Binary<  11011111>::value, 8}, // WHorse
        {Binary<  11111111>::value, 8}, // WDragon
    };
    
    // 盤上の bit 数 - 1 で表現出来るようにする。持ち駒があると、盤上には Empty の 1 bit が増えるので、
    // これで局面の bit 数が固定化される。
    const HuffmanCode HuffmanCodedPos::handCodeTable[HandPieceNum][ColorNum] = {
        {{Binary<        0>::value, 3}, {Binary<      100>::value, 3}}, // HPawn
        {{Binary<        1>::value, 5}, {Binary<    10001>::value, 5}}, // HLance
        {{Binary<       11>::value, 5}, {Binary<    10011>::value, 5}}, // HKnight
        {{Binary<      101>::value, 5}, {Binary<    10101>::value, 5}}, // HSilver
        {{Binary<      111>::value, 5}, {Binary<    10111>::value, 5}}, // HGold
        {{Binary<    11111>::value, 7}, {Binary<  1011111>::value, 7}}, // HBishop
        {{Binary<   111111>::value, 7}, {Binary<  1111111>::value, 7}}, // HRook
    };
    
    HuffmanCodeToPieceHash HuffmanCodedPos::boardCodeToPieceHash;
    HuffmanCodeToPieceHash HuffmanCodedPos::handCodeToPieceHash;
    
    const CharToPieceUSI g_charToPieceUSI;
    
    namespace {
        const char* PieceToCharCSATable[PieceNone] = {
            " * ", "+FU", "+KY", "+KE", "+GI", "+KA", "+HI", "+KI", "+OU", "+TO", "+NY", "+NK", "+NG", "+UM", "+RY", "", "",
            "-FU", "-KY", "-KE", "-GI", "-KA", "-HI", "-KI", "-OU", "-TO", "-NY", "-NK", "-NG", "-UM", "-RY"
        };
        inline const char* pieceToCharCSA(const Piece pc) {
            return PieceToCharCSATable[pc];
        }
        const char* PieceToCharUSITable[PieceNone] = {
            "", "P", "L", "N", "S", "B", "R", "G", "K", "+P", "+L", "+N", "+S", "+B", "+R", "", "",
            "p", "l", "n", "s", "b", "r", "g", "k", "+p", "+l", "+n", "+s", "+b", "+r"
        };
        inline const char* pieceToCharUSI(const Piece pc) {
            return PieceToCharUSITable[pc];
        }
    }
    
    CheckInfo::CheckInfo(const Position& pos) {
        const Color them = oppositeColor(pos.turn());
        const Square ksq = pos.kingSquare(them);
        
        pinned = pos.pinnedBB();
        dcBB = pos.discoveredCheckBB();
        
        checkBB[Pawn     ] = pos.attacksFrom<Pawn  >(them, ksq);
        checkBB[Lance    ] = pos.attacksFrom<Lance >(them, ksq);
        checkBB[Knight   ] = pos.attacksFrom<Knight>(them, ksq);
        checkBB[Silver   ] = pos.attacksFrom<Silver>(them, ksq);
        checkBB[Bishop   ] = pos.attacksFrom<Bishop>(ksq);
        checkBB[Rook     ] = pos.attacksFrom<Rook  >(ksq);
        checkBB[Gold     ] = pos.attacksFrom<Gold  >(them, ksq);
        checkBB[King     ] = allZeroBB();
        // todo: ここで AVX2 使えそう。
        //       checkBB のreadアクセスは switch (pt) で場合分けして、余計なコピー減らした方が良いかも。
        checkBB[ProPawn  ] = checkBB[Gold];
        checkBB[ProLance ] = checkBB[Gold];
        checkBB[ProKnight] = checkBB[Gold];
        checkBB[ProSilver] = checkBB[Gold];
        checkBB[Horse    ] = checkBB[Bishop] | pos.attacksFrom<King>(ksq);
        checkBB[Dragon   ] = checkBB[Rook  ] | pos.attacksFrom<King>(ksq);
    }
    
    Bitboard Position::attacksFrom(const PieceType pt, const Color c, const Square sq, const Bitboard& occupied) {
        switch (pt) {
            case Occupied:  return allZeroBB();
            case Pawn:      return pawnAttack(c, sq);
            case Lance:     return lanceAttack(c, sq, occupied);
            case Knight:    return knightAttack(c, sq);
            case Silver:    return silverAttack(c, sq);
            case Bishop:    return bishopAttack(sq, occupied);
            case Rook:      return rookAttack(sq, occupied);
            case Gold:
            case ProPawn:
            case ProLance:
            case ProKnight:
            case ProSilver: return goldAttack(c, sq);
            case King:      return kingAttack(sq);
            case Horse:     return horseAttack(sq, occupied);
            case Dragon:    return dragonAttack(sq, occupied);
            default:        UNREACHABLE; return allOneBB();
        }
    }
    
    // 実際に指し手が合法手かどうか判定
    // 連続王手の千日手は排除しない。
    // 確実に駒打ちではないときは、MUSTNOTDROP == true とする。
    // 確実に玉の移動で無いときは、FROMMUSTNOTKING == true とする。英語として正しい？
    // 遠隔駒で王手されているとき、その駒の利きがある場所に逃げる手を検出出来ない場合があるので、
    // そのような手を指し手生成してはいけない。
    template <bool MUSTNOTDROP, bool FROMMUSTNOTKING>
    bool Position::pseudoLegalMoveIsLegal(const Move move, const Bitboard& pinned) const {
        // 駒打ちは、打ち歩詰めや二歩は指し手生成時や、killerをMovePicker::nextMove() 内で排除しているので、常に合法手
        // (連続王手の千日手は省いていないけれど。)
        if (!MUSTNOTDROP && move.isDrop())
            return true;
        // assert(!move.isDrop());
        if(move.isDrop()){
            printf("error, assert(!move.isDrop())\n");
        }
        
        const Color us = turn();
        const Square from = move.from();
        
        if (!FROMMUSTNOTKING && pieceToPieceType(piece(from)) == King) {
            const Color them = oppositeColor(us);
            // 玉の移動先に相手の駒の利きがあれば、合法手でないので、false
            return !attackersToIsAny(them, move.to());
        }
        // 玉以外の駒の移動
        return !isPinnedIllegal(from, move.to(), kingSquare(us), pinned);
    }
    
    template bool Position::pseudoLegalMoveIsLegal<false, false>(const Move move, const Bitboard& pinned) const;
    template bool Position::pseudoLegalMoveIsLegal<false, true >(const Move move, const Bitboard& pinned) const;
    template bool Position::pseudoLegalMoveIsLegal<true,  false>(const Move move, const Bitboard& pinned) const;
    
    bool Position::pseudoLegalMoveIsEvasion(const Move move, const Bitboard& pinned) const {
        // assert(isOK());
        if(!isOK()){
            printf("error, assert(isOK())\n");
        }
        
        // 玉の移動
        if (move.pieceTypeFrom() == King) {
            // 遠隔駒で王手されたとき、王手している遠隔駒の利きには移動しないように指し手を生成している。
            // その為、移動先に他の駒の利きが無いか調べるだけで良い。
            const bool canMove = !attackersToIsAny(oppositeColor(turn()), move.to());
            
            // TODO assert(canMove == (pseudoLegalMoveIsLegal<false, false>(move, pinned)));
            if(!(canMove == (pseudoLegalMoveIsLegal<false, false>(move, pinned)))){
                printf("error, assert(canMove == (pseudoLegalMoveIsLegal<false, false>(move, pinned)))\n");
            }
            
            return canMove;
        }
        
        // 玉の移動以外
        Bitboard target = checkersBB();
        const Square checkSq = target.firstOneFromSQ11();
        
        if (target)
            // 両王手のとき、玉の移動以外の手は指せない。
            return false;
        
        const Color us = turn();
        const Square to = move.to();
        // 移動、又は打った駒が、王手をさえぎるか、王手している駒を取る必要がある。
        target = betweenBB(checkSq, kingSquare(us)) | checkersBB();
        return target.isSet(to) && pseudoLegalMoveIsLegal<false, true>(move, pinned);
    }
    
    // Searching: true なら探索時に内部で生成した手の合法手判定を行う。
    //            ttMove で hash 値が衝突した時などで、大駒の不成など明らかに価値の低い手が生成される事がある。
    //            これは非合法手として省いて良い。
    //            false なら、外部入力の合法手判定なので、ルールと同一の条件になる事が望ましい。
    template <bool Searching> bool Position::moveIsPseudoLegal(const Move move) const {
        const Color us = turn();
        const Color them = oppositeColor(us);
        const Square to = move.to();
        
        if (move.isDrop()) {
            const PieceType ptFrom = move.pieceTypeDropped();
            if (!hand(us).exists(pieceTypeToHandPiece(ptFrom)) || piece(to) != Empty)
                return false;
            
            if (inCheck()) {
                // 王手されているので、合駒でなければならない。
                Bitboard target = checkersBB();
                const Square checksq = target.firstOneFromSQ11();
                
                if (target)
                    // 両王手は合駒出来無い。
                    return false;
                
                target = betweenBB(checksq, kingSquare(us));
                if (!target.isSet(to))
                    // 玉と、王手した駒との間に駒を打っていない。
                    return false;
            }
            
            if (ptFrom == Pawn) {
                if ((bbOf(Pawn, us) & fileMask(makeFile(to))))
                    // 二歩
                    return false;
                const SquareDelta TDeltaN = (us == Black ? DeltaN : DeltaS);
                if (to + TDeltaN == kingSquare(them) && isPawnDropCheckMate(us, to))
                    // 王手かつ打ち歩詰め
                    return false;
            }
        }
        else {
            const Square from = move.from();
            const PieceType ptFrom = move.pieceTypeFrom();
            if (piece(from) != colorAndPieceTypeToPiece(us, ptFrom) || bbOf(us).isSet(to))
                return false;
            
            if (!attacksFrom(ptFrom, us, from).isSet(to))
                return false;
            
            if (Searching) {
                switch (ptFrom) {
                    case Pawn  :
                        if (move.isPromotion()) {
                            if (!canPromote(us, makeRank(to)))
                                return false;
                        }
                        else if (canPromote(us, makeRank(to)))
                            return false;
                        break;
                    case Lance :
                        if (move.isPromotion()) {
                            if (!canPromote(us, makeRank(to)))
                                return false;
                        }
                        else {
                            // 1段目の不成は非合法なので省く。2段目の不成と3段目の駒を取らない不成もついでに省く。
                            const Rank toRank = makeRank(to);
                            if (us == Black ? isInFrontOf<Black, Rank3, Rank7>(toRank) : isInFrontOf<White, Rank3, Rank7>(toRank))
                                return false;
                            if (canPromote(us, toRank) && !move.isCapture())
                                return false;
                        }
                        break;
                    case Knight:
                        // hash 値が衝突して別の局面の合法手の ttMove が入力されても、桂馬である事は確定。(桂馬は移動元、移動先が特殊なので。)
                        // よって、行きどころの無い駒になる move は生成されない。
                        // 特にチェックすべき事は無いので、break
                        break;
                    case Silver: case Bishop: case Rook  :
                        if (move.isPromotion())
                            if (!canPromote(us, makeRank(to)) && !canPromote(us, makeRank(from)))
                                return false;
                        break;
                    default: // 成れない駒
                        if (move.isPromotion())
                            return false;
                }
            }
            
            if (inCheck()) {
                if (ptFrom == King) {
                    Bitboard occ = occupiedBB();
                    occ.clearBit(from);
                    if (attackersToIsAny(them, to, occ))
                        // 王手から逃げていない。
                        return false;
                }
                else {
                    // 玉以外の駒を移動させたとき。
                    Bitboard target = checkersBB();
                    const Square checksq = target.firstOneFromSQ11();
                    
                    if (target)
                        // 両王手なので、玉が逃げない手は駄目
                        return false;
                    
                    target = betweenBB(checksq, kingSquare(us)) | checkersBB();
                    if (!target.isSet(to))
                        // 玉と、王手した駒との間に移動するか、王手した駒を取る以外は駄目。
                        return false;
                }
            }
        }
        
        return true;
    }
    
    template bool Position::moveIsPseudoLegal<true >(const Move move) const;
    template bool Position::moveIsPseudoLegal<false>(const Move move) const;

    // 過去(又は現在)に生成した指し手が現在の局面でも有効か判定。
    // あまり速度が要求される場面で使ってはいけない。
    bool Position::moveIsLegal(const Move move) const {
        return MoveList<LegalAll>(*this).contains(move);
    }
    
    // 局面の更新
    void Position::doMove(const Move move, StateInfo& newSt) {
        const CheckInfo ci(*this);
        doMove(move, newSt, ci, moveGivesCheck(move, ci));
    }
    
    // 局面の更新
    void Position::doMove(const Move move, StateInfo& newSt, const CheckInfo& ci, const bool moveIsCheck) {
        // TODO assert(isOK());
        if(!isOK()){
            printf("error, assert(isOK())\n");
        }
        // TODO assert(move);
        if(!move){
            printf("error, assert(move)\n");
        }
        // TODO assert(&newSt != st_);
        if(!(&newSt != st_)){
            printf("error, assert(&newSt != st_)\n");
        }
        
        Key boardKey = getBoardKey();
        Key handKey = getHandKey();
        boardKey ^= zobTurn();
        
        memcpy(&newSt, st_, offsetof(StateInfo, boardKey));
        newSt.previous = st_;
        st_ = &newSt;
        
        st_->cl.size = 1;
        
        const Color us = turn();
        const Square to = move.to();
        const PieceType ptCaptured = move.cap();
        PieceType ptTo;
        if (move.isDrop()) {
            ptTo = move.pieceTypeDropped();
            const HandPiece hpTo = pieceTypeToHandPiece(ptTo);
            
            handKey -= zobHand(hpTo, us);
            boardKey += zobrist(ptTo, to, us);
            
            prefetch(csearcher()->tt.firstEntry(boardKey + handKey));
            
            const int32_t handnum = hand(us).numOf(hpTo);
            const int32_t listIndex = evalList_.squareHandToList[HandPieceToSquareHand[us][hpTo] + handnum];
            const Piece pcTo = colorAndPieceTypeToPiece(us, ptTo);
            st_->cl.listindex[0] = listIndex;
            st_->cl.clistpair[0].oldlist[0] = evalList_.list0[listIndex];
            st_->cl.clistpair[0].oldlist[1] = evalList_.list1[listIndex];
            
            evalList_.list0[listIndex] = kppArray[pcTo         ] + (EvalIndex)to;
            evalList_.list1[listIndex] = kppArray[inverse(pcTo)] + (EvalIndex)inverse(to);
            evalList_.listToSquareHand[listIndex] = to;
            evalList_.squareHandToList[to] = listIndex;
            
            st_->cl.clistpair[0].newlist[0] = evalList_.list0[listIndex];
            st_->cl.clistpair[0].newlist[1] = evalList_.list1[listIndex];
            
            hand_[us].minusOne(hpTo);
            xorBBs(ptTo, to, us);
            piece_[to] = colorAndPieceTypeToPiece(us, ptTo);
            
            if (moveIsCheck) {
                // Direct checks
                st_->checkersBB = setMaskBB(to);
                st_->continuousCheck[us] += 2;
            }
            else {
                st_->checkersBB = allZeroBB();
                st_->continuousCheck[us] = 0;
            }
        }
        else {
            const Square from = move.from();
            const PieceType ptFrom = move.pieceTypeFrom();
            ptTo = move.pieceTypeTo(ptFrom);
            
            byTypeBB_[ptFrom].xorBit(from);
            byTypeBB_[ptTo].xorBit(to);
            byColorBB_[us].xorBit(from, to);
            piece_[from] = Empty;
            piece_[to] = colorAndPieceTypeToPiece(us, ptTo);
            boardKey -= zobrist(ptFrom, from, us);
            boardKey += zobrist(ptTo, to, us);
            
            if (ptCaptured) {
                // 駒を取ったとき
                const HandPiece hpCaptured = pieceTypeToHandPiece(ptCaptured);
                const Color them = oppositeColor(us);
                
                boardKey -= zobrist(ptCaptured, to, them);
                handKey += zobHand(hpCaptured, us);
                
                byTypeBB_[ptCaptured].xorBit(to);
                byColorBB_[them].xorBit(to);
                
                hand_[us].plusOne(hpCaptured);
                const int32_t toListIndex = evalList_.squareHandToList[to];
                st_->cl.listindex[1] = toListIndex;
                st_->cl.clistpair[1].oldlist[0] = evalList_.list0[toListIndex];
                st_->cl.clistpair[1].oldlist[1] = evalList_.list1[toListIndex];
                st_->cl.size = 2;
                
                const int32_t handnum = hand(us).numOf(hpCaptured);
                evalList_.list0[toListIndex] = kppHandArray[us  ][hpCaptured] + handnum;
                evalList_.list1[toListIndex] = kppHandArray[them][hpCaptured] + handnum;
                const Square squarehand = HandPieceToSquareHand[us][hpCaptured] + handnum;
                evalList_.listToSquareHand[toListIndex] = squarehand;
                evalList_.squareHandToList[squarehand]  = toListIndex;
                
                st_->cl.clistpair[1].newlist[0] = evalList_.list0[toListIndex];
                st_->cl.clistpair[1].newlist[1] = evalList_.list1[toListIndex];
                
                st_->material += (us == Black ? capturePieceScore(ptCaptured) : -capturePieceScore(ptCaptured));
            }
            prefetch(csearcher()->tt.firstEntry(boardKey + handKey));
            // Occupied は to, from の位置のビットを操作するよりも、
            // Black と White の or を取る方が速いはず。
            byTypeBB_[Occupied] = bbOf(Black) | bbOf(White);
            
            if (ptTo == King)
                kingSquare_[us] = to;
            else {
                const Piece pcTo = colorAndPieceTypeToPiece(us, ptTo);
                const int32_t fromListIndex = evalList_.squareHandToList[from];
                
                st_->cl.listindex[0] = fromListIndex;
                st_->cl.clistpair[0].oldlist[0] = evalList_.list0[fromListIndex];
                st_->cl.clistpair[0].oldlist[1] = evalList_.list1[fromListIndex];
                
                evalList_.list0[fromListIndex] = kppArray[pcTo         ] + (EvalIndex)to;
                evalList_.list1[fromListIndex] = kppArray[inverse(pcTo)] + (EvalIndex)inverse(to);
                evalList_.listToSquareHand[fromListIndex] = to;
                evalList_.squareHandToList[to] = fromListIndex;
                
                st_->cl.clistpair[0].newlist[0] = evalList_.list0[fromListIndex];
                st_->cl.clistpair[0].newlist[1] = evalList_.list1[fromListIndex];
            }
            
            if (move.isPromotion())
                st_->material += (us == Black ? (pieceScore(ptTo) - pieceScore(ptFrom)) : -(pieceScore(ptTo) - pieceScore(ptFrom)));
            
            if (moveIsCheck) {
                // Direct checks
                st_->checkersBB = ci.checkBB[ptTo] & setMaskBB(to);
                
                // Discovery checks
                const Square ksq = kingSquare(oppositeColor(us));
                if (isDiscoveredCheck(from, to, ksq, ci.dcBB)) {
                    switch (squareRelation(from, ksq)) {
                        case DirecMisc:
                            // TODO assert(false);
                            printf("error, assert(false)\n");
                            break; // 最適化の為のダミー
                        case DirecFile:
                            // from の位置から縦に利きを調べると相手玉と、空き王手している駒に当たっているはず。味方の駒が空き王手している駒。
                            st_->checkersBB |= rookAttackFile(from, occupiedBB()) & bbOf(us);
                            break;
                        case DirecRank:
                            st_->checkersBB |= attacksFrom<Rook>(ksq) & bbOf(Rook, Dragon, us);
                            break;
                        case DirecDiagNESW: case DirecDiagNWSE:
                            st_->checkersBB |= attacksFrom<Bishop>(ksq) & bbOf(Bishop, Horse, us);
                            break;
                        default: UNREACHABLE;
                    }
                }
                st_->continuousCheck[us] += 2;
            }
            else {
                st_->checkersBB = allZeroBB();
                st_->continuousCheck[us] = 0;
            }
        }
        goldsBB_ = bbOf(Gold, ProPawn, ProLance, ProKnight, ProSilver);
        
        st_->boardKey = boardKey;
        st_->handKey = handKey;
        ++st_->pliesFromNull;
        
        turn_ = oppositeColor(us);
        st_->hand = hand(turn());
        
        // TODO assert(isOK());
        if(!isOK()){
            printf("error, assert(isOK())\n");
        }
    }
    
    void Position::undoMove(const Move move) {
        // assert(isOK());
        if(!isOK()){
            printf("error, assert(isOK())\n");
        }
        // assert(move);
        if(!move){
            printf("error, assert(move)\n");
        }
        
        const Color them = turn();
        const Color us = oppositeColor(them);
        const Square to = move.to();
        turn_ = us;
        // ここで先に turn_ を戻したので、以下、move は us の指し手とする。
        if (move.isDrop()) {
            const PieceType ptTo = move.pieceTypeDropped();
            byTypeBB_[ptTo].xorBit(to);
            byColorBB_[us].xorBit(to);
            piece_[to] = Empty;
            
            const HandPiece hp = pieceTypeToHandPiece(ptTo);
            hand_[us].plusOne(hp);
            
            const int32_t toListIndex  = evalList_.squareHandToList[to];
            const int32_t handnum = hand(us).numOf(hp);
            evalList_.list0[toListIndex] = kppHandArray[us  ][hp] + handnum;
            evalList_.list1[toListIndex] = kppHandArray[them][hp] + handnum;
            const Square squarehand = HandPieceToSquareHand[us][hp] + handnum;
            evalList_.listToSquareHand[toListIndex] = squarehand;
            evalList_.squareHandToList[squarehand]  = toListIndex;
        }
        else {
            const Square from = move.from();
            const PieceType ptFrom = move.pieceTypeFrom();
            const PieceType ptTo = move.pieceTypeTo(ptFrom);
            const PieceType ptCaptured = move.cap(); // todo: st_->capturedType 使えば良い。
            
            if (ptTo == King)
                kingSquare_[us] = from;
            else {
                const Piece pcFrom = colorAndPieceTypeToPiece(us, ptFrom);
                const int32_t toListIndex = evalList_.squareHandToList[to];
                evalList_.list0[toListIndex] = kppArray[pcFrom         ] + (EvalIndex)from;
                evalList_.list1[toListIndex] = kppArray[inverse(pcFrom)] + (EvalIndex)inverse(from);
                evalList_.listToSquareHand[toListIndex] = from;
                evalList_.squareHandToList[from] = toListIndex;
            }
            
            if (ptCaptured) {
                // 駒を取ったとき
                byTypeBB_[ptCaptured].xorBit(to);
                byColorBB_[them].xorBit(to);
                const HandPiece hpCaptured = pieceTypeToHandPiece(ptCaptured);
                const Piece pcCaptured = colorAndPieceTypeToPiece(them, ptCaptured);
                piece_[to] = pcCaptured;
                
                const int32_t handnum = hand(us).numOf(hpCaptured);
                const int32_t toListIndex = evalList_.squareHandToList[HandPieceToSquareHand[us][hpCaptured] + handnum];
                evalList_.list0[toListIndex] = kppArray[pcCaptured         ] + (EvalIndex)to;
                evalList_.list1[toListIndex] = kppArray[inverse(pcCaptured)] + (EvalIndex)inverse(to);
                evalList_.listToSquareHand[toListIndex] = to;
                evalList_.squareHandToList[to] = toListIndex;
                
                hand_[us].minusOne(hpCaptured);
            }
            else
                // 駒を取らないときは、colorAndPieceTypeToPiece(us, ptCaptured) は 0 または 16 になる。
                // 16 になると困るので、駒を取らないときは明示的に Empty にする。
                piece_[to] = Empty;
            byTypeBB_[ptFrom].xorBit(from);
            byTypeBB_[ptTo].xorBit(to);
            byColorBB_[us].xorBit(from, to);
            piece_[from] = colorAndPieceTypeToPiece(us, ptFrom);
        }
        // Occupied は to, from の位置のビットを操作するよりも、
        // Black と White の or を取る方が速いはず。
        byTypeBB_[Occupied] = bbOf(Black) | bbOf(White);
        goldsBB_ = bbOf(Gold, ProPawn, ProLance, ProKnight, ProSilver);
        
        // key などは StateInfo にまとめられているので、
        // previous のポインタを st_ に代入するだけで良い。
        st_ = st_->previous;
        
        // TODO assert(isOK());
        if(!isOK()){
            printf("error: position not ok\n");
        }
    }
    
    namespace {
        // SEE の順番
        template <PieceType PT> struct SEENextPieceType {}; // これはインスタンス化しない。
        template <> struct SEENextPieceType<Pawn     > { static const PieceType value = Lance;     };
        template <> struct SEENextPieceType<Lance    > { static const PieceType value = Knight;    };
        template <> struct SEENextPieceType<Knight   > { static const PieceType value = ProPawn;   };
        template <> struct SEENextPieceType<ProPawn  > { static const PieceType value = ProLance;  };
        template <> struct SEENextPieceType<ProLance > { static const PieceType value = ProKnight; };
        template <> struct SEENextPieceType<ProKnight> { static const PieceType value = Silver;    };
        template <> struct SEENextPieceType<Silver   > { static const PieceType value = ProSilver; };
        template <> struct SEENextPieceType<ProSilver> { static const PieceType value = Gold;      };
        template <> struct SEENextPieceType<Gold     > { static const PieceType value = Bishop;    };
        template <> struct SEENextPieceType<Bishop   > { static const PieceType value = Horse;     };
        template <> struct SEENextPieceType<Horse    > { static const PieceType value = Rook;      };
        template <> struct SEENextPieceType<Rook     > { static const PieceType value = Dragon;    };
        template <> struct SEENextPieceType<Dragon   > { static const PieceType value = King;      };
        
        template <PieceType PT> FORCE_INLINE PieceType nextAttacker(const Position& pos, const Square to, const Bitboard& opponentAttackers,
                                                                    Bitboard& occupied, Bitboard& attackers, const Color turn)
        {
            if (opponentAttackers.andIsAny(pos.bbOf(PT))) {
                const Bitboard bb = opponentAttackers & pos.bbOf(PT);
                const Square from = bb.constFirstOneFromSQ11();
                occupied.xorBit(from);
                // todo: 実際に移動した方向を基にattackersを更新すれば、template, inline を使用しなくても良さそう。
                //       その場合、キャッシュに乗りやすくなるので逆に速くなるかも。
                if (PT == Pawn || PT == Lance)
                    attackers |= (lanceAttack(oppositeColor(turn), to, occupied) & (pos.bbOf(Rook, Dragon) | pos.bbOf(Lance, turn)));
                if (PT == Gold || PT == ProPawn || PT == ProLance || PT == ProKnight || PT == ProSilver || PT == Horse || PT == Dragon)
                    attackers |= (lanceAttack(oppositeColor(turn), to, occupied) & pos.bbOf(Lance, turn))
                    | (lanceAttack(turn, to, occupied) & pos.bbOf(Lance, oppositeColor(turn)))
                    | (rookAttack(to, occupied) & pos.bbOf(Rook, Dragon))
                    | (bishopAttack(to, occupied) & pos.bbOf(Bishop, Horse));
                if (PT == Silver)
                    attackers |= (lanceAttack(oppositeColor(turn), to, occupied) & pos.bbOf(Lance, turn))
                    | (rookAttack(to, occupied) & pos.bbOf(Rook, Dragon))
                    | (bishopAttack(to, occupied) & pos.bbOf(Bishop, Horse));
                if (PT == Bishop)
                    attackers |= (bishopAttack(to, occupied) & pos.bbOf(Bishop, Horse));
                if (PT == Rook)
                    attackers |= (lanceAttack(oppositeColor(turn), to, occupied) & pos.bbOf(Lance, turn))
                    | (lanceAttack(turn, to, occupied) & pos.bbOf(Lance, oppositeColor(turn)))
                    | (rookAttack(to, occupied) & pos.bbOf(Rook, Dragon));
                
                if (PT == Pawn || PT == Lance || PT == Knight)
                    if (canPromote(turn, makeRank(to)))
                        return PT + PTPromote;
                if (PT == Silver || PT == Bishop || PT == Rook)
                    if (canPromote(turn, makeRank(to)) || canPromote(turn, makeRank(from)))
                        return PT + PTPromote;
                return PT;
            }
            return nextAttacker<SEENextPieceType<PT>::value>(pos, to, opponentAttackers, occupied, attackers, turn);
        }
        template <> FORCE_INLINE PieceType nextAttacker<King>(const Position&, const Square, const Bitboard&,
                                                              Bitboard&, Bitboard&, const Color)
        {
            return King;
        }
    }
    
    Score Position::see(const Move move, const int32_t asymmThreshold) const {
        const Square to = move.to();
        Square from;
        PieceType ptCaptured;
        Bitboard occ = occupiedBB();
        Bitboard attackers;
        Bitboard opponentAttackers;
        Color turn = oppositeColor(this->turn());
        Score swapList[32];
        if (move.isDrop()) {
            opponentAttackers = attackersTo(turn, to, occ);
            if (!opponentAttackers)
                return ScoreZero;
            attackers = opponentAttackers | attackersTo(oppositeColor(turn), to, occ);
            swapList[0] = ScoreZero;
            ptCaptured = move.pieceTypeDropped();
        }
        else {
            from = move.from();
            occ.xorBit(from);
            opponentAttackers = attackersTo(turn, to, occ);
            if (!opponentAttackers) {
                if (move.isPromotion()) {
                    const PieceType ptFrom = move.pieceTypeFrom();
                    return capturePieceScore(move.cap()) + promotePieceScore(ptFrom);
                }
                return capturePieceScore(move.cap());
            }
            attackers = opponentAttackers | attackersTo(oppositeColor(turn), to, occ);
            swapList[0] = capturePieceScore(move.cap());
            ptCaptured = move.pieceTypeFrom();
            if (move.isPromotion()) {
                const PieceType ptFrom = move.pieceTypeFrom();
                swapList[0] += promotePieceScore(ptFrom);
                ptCaptured += PTPromote;
            }
        }

        int32_t slIndex = 1;
        do {
            swapList[slIndex] = -swapList[slIndex - 1] + capturePieceScore(ptCaptured);
            
            ptCaptured = nextAttacker<Pawn>(*this, to, opponentAttackers, occ, attackers, turn);
            
            attackers &= occ;
            ++slIndex;
            turn = oppositeColor(turn);
            opponentAttackers = attackers & bbOf(turn);
            
            if (ptCaptured == King) {
                if (opponentAttackers)
                    swapList[slIndex++] = CaptureKingScore;
                break;
            }
        } while (opponentAttackers);
        
        if (asymmThreshold) {
            for (int32_t i = 0; i < slIndex; i += 2) {
                if (swapList[i] < asymmThreshold)
                    swapList[i] = -CaptureKingScore;
            }
        }
        
        // nega max 的に駒の取り合いの点数を求める。
        while (--slIndex)
            swapList[slIndex-1] = min(-swapList[slIndex], swapList[slIndex-1]);
        return swapList[0];
    }
    
    Score Position::seeSign(const Move move) const {
        if (move.isCapture()) {
            const PieceType ptFrom = move.pieceTypeFrom();
            const Square to = move.to();
            if (capturePieceScore(ptFrom) <= capturePieceScore(piece(to)))
                return static_cast<Score>(1);
        }
        return see(move);
    }
    
    namespace {
        // them(相手) 側の玉が逃げられるか。
        // sq : 王手した相手の駒の位置。紐付きか、桂馬の位置とする。よって、玉は sq には行けない。
        // bb : sq の利きのある場所のBitboard。よって、玉は bb のビットが立っている場所には行けない。
        // sq と ksq の位置の Occupied Bitboard のみは、ここで更新して評価し、元に戻す。
        // (実際にはテンポラリのOccupied Bitboard を使うので、元には戻さない。)
        bool canKingEscape(const Position& pos, const Color us, const Square sq, const Bitboard& bb) {
            const Color them = oppositeColor(us);
            const Square ksq = pos.kingSquare(them);
            Bitboard kingMoveBB = bb.notThisAnd(pos.bbOf(them).notThisAnd(kingAttack(ksq)));
            kingMoveBB.clearBit(sq); // sq には行けないので、クリアする。xorBit(sq)ではダメ。
            
            if (kingMoveBB) {
                Bitboard tempOccupied = pos.occupiedBB();
                tempOccupied.setBit(sq);
                tempOccupied.clearBit(ksq);
                do {
                    const Square to = kingMoveBB.firstOneFromSQ11();
                    // 玉の移動先に、us 側の利きが無ければ、true
                    if (!pos.attackersToIsAny(us, to, tempOccupied))
                        return true;
                } while (kingMoveBB);
            }
            // 玉の移動先が無い。
            return false;
        }
        // them(相手) 側の玉以外の駒が sq にある us 側の駒を取れるか。
        bool canPieceCapture(const Position& pos, const Color them, const Square sq, const Bitboard& dcBB) {
            // 玉以外で打った駒を取れる相手側の駒の Bitboard
            Bitboard fromBB = pos.attackersToExceptKing(them, sq);
            
            if (fromBB) {
                const Square ksq = pos.kingSquare(them);
                do {
                    const Square from = fromBB.firstOneFromSQ11();
                    if (!pos.isDiscoveredCheck(from, sq, ksq, dcBB))
                        // them 側から見て、pin されていない駒で、打たれた駒を取れるので、true
                        return true;
                } while (fromBB);
            }
            // 玉以外の駒で、打った駒を取れない。
            return false;
        }
        
        // pos.discoveredCheckBB<false>() を遅延評価するバージョン。
        bool canPieceCapture(const Position& pos, const Color them, const Square sq) {
            Bitboard fromBB = pos.attackersToExceptKing(them, sq);
            
            if (fromBB) {
                const Square ksq = pos.kingSquare(them);
                const Bitboard dcBB = pos.discoveredCheckBB<false>();
                do {
                    const Square from = fromBB.firstOneFromSQ11();
                    if (!pos.isDiscoveredCheck(from, sq, ksq, dcBB))
                        // them 側から見て、pin されていない駒で、打たれた駒を取れるので、true
                        return true;
                } while (fromBB);
            }
            // 玉以外の駒で、打った駒を取れない。
            return false;
        }
    }
    
    // us が sq へ歩を打ったとき、them の玉が詰むか。
    // us が sq へ歩を打つのは王手であると仮定する。
    // 打ち歩詰めのとき、true を返す。
    bool Position::isPawnDropCheckMate(const Color us, const Square sq) const {
        const Color them = oppositeColor(us);
        // 玉以外の駒で、打たれた歩が取れるなら、打ち歩詰めではない。
        if (canPieceCapture(*this, them, sq))
            return false;
        // todo: ここで玉の位置を求めるのは、上位で求めたものと2重になるので無駄。後で整理すること。
        const Square ksq = kingSquare(them);
        
        // 玉以外で打った歩を取れないとき、玉が歩を取るか、玉が逃げるか。
        
        // 利きを求める際に、occupied の歩を打った位置の bit を立てた Bitboard を使用する。
        // ここでは歩の Bitboard は更新する必要がない。
        // color の Bitboard も更新する必要がない。(相手玉が動くとき、こちらの打った歩で玉を取ることは無い為。)
        const Bitboard tempOccupied = occupiedBB() | setMaskBB(sq);
        Bitboard kingMoveBB = bbOf(them).notThisAnd(kingAttack(ksq));
        
        // 少なくとも歩を取る方向には玉が動けるはずなので、do while を使用。
        // assert(kingMoveBB);
        if(!kingMoveBB){
            printf("error, assert(kingMoveBB)\n");
        }
        do {
            const Square to = kingMoveBB.firstOneFromSQ11();
            if (!attackersToIsAny(us, to, tempOccupied))
                // 相手玉の移動先に自駒の利きがないなら、打ち歩詰めではない。
                return false;
        } while (kingMoveBB);
        
        return true;
    }
    
    inline void Position::xorBBs(const PieceType pt, const Square sq, const Color c) {
        byTypeBB_[Occupied].xorBit(sq);
        byTypeBB_[pt].xorBit(sq);
        byColorBB_[c].xorBit(sq);
    }
    
    // 相手玉が1手詰みかどうかを判定。
    // 1手詰みなら、詰みに至る指し手の一部の情報(from, to のみとか)を返す。
    // 1手詰みでないなら、Move::moveNone() を返す。
    // Bitboard の状態を途中で更新する為、const 関数ではない。(更新後、元に戻すが。)
    template <Color US> Move Position::mateMoveIn1Ply() {
        // printf("mateMoveIn1Ply: %d\n", US);
        const Color Them = oppositeColor(US);
        const Square ksq = kingSquare(Them);
        const SquareDelta TDeltaS = (US == Black ? DeltaS : DeltaN);
        
        // assert(!attackersToIsAny(Them, kingSquare(US)));
        if(attackersToIsAny(Them, kingSquare(US))){
            printf("error, assert(!attackersToIsAny(Them, kingSquare(US)))\n");
        }
        
        // 駒打ちを調べる。
        const Bitboard dropTarget = nOccupiedBB(); // emptyBB() ではないので注意して使うこと。
        const Hand ourHand = hand(US);
        // 王手する前の状態の dcBB。
        // 間にある駒は相手側の駒。
        // 駒打ちのときは、打った後も、打たれる前の状態の dcBB を使用する。
        const Bitboard dcBB_betweenIsThem = discoveredCheckBB<false>();
        
        // 飛車打ち
        if (ourHand.exists<HRook>()) {
            // 合駒されるとややこしいので、3手詰み関数の中で調べる。
            // ここでは離れた位置から王手するのは考えない。
            Bitboard toBB = dropTarget & rookStepAttacks(ksq);
            while (toBB) {
                const Square to = toBB.firstOneFromSQ11();
                // 駒を打った場所に自駒の利きがあるか。(無ければ玉で取られて詰まない)
                if (attackersToIsAny(US, to)) {
                    // 玉が逃げられず、他の駒で取ることも出来ないか
                    if (!canKingEscape(*this, US, to, rookAttackToEdge(to))
                        && !canPieceCapture(*this, Them, to, dcBB_betweenIsThem))
                    {
                        return makeDropMove(Rook, to);
                    }
                }
            }
        }
        // 香車打ち
        // 飛車で詰まなければ香車でも詰まないので、else if を使用。
        // 玉が 9(1) 段目にいれば香車で王手出来無いので、それも省く。
        else if (ourHand.exists<HLance>() && isInFrontOf<US, Rank9, Rank1>(makeRank(ksq))) {
            const Square to = ksq + TDeltaS;
            if (piece(to) == Empty && attackersToIsAny(US, to)) {
                if (!canKingEscape(*this, US, to, lanceAttackToEdge(US, to))
                    && !canPieceCapture(*this, Them, to, dcBB_betweenIsThem))
                {
                    return makeDropMove(Lance, to);
                }
            }
        }
        
        // 角打ち
        if (ourHand.exists<HBishop>()) {
            Bitboard toBB = dropTarget & bishopStepAttacks(ksq);
            while (toBB) {
                const Square to = toBB.firstOneFromSQ11();
                if (attackersToIsAny(US, to)) {
                    if (!canKingEscape(*this, US, to, bishopAttackToEdge(to))
                        && !canPieceCapture(*this, Them, to, dcBB_betweenIsThem))
                    {
                        return makeDropMove(Bishop, to);
                    }
                }
            }
        }
        
        // 金打ち
        if (ourHand.exists<HGold>()) {
            Bitboard toBB;
            if (ourHand.exists<HRook>())
                // 飛車打ちを先に調べたので、尻金だけは省く。
                toBB = dropTarget & (goldAttack(Them, ksq) ^ pawnAttack(US, ksq));
            else
                toBB = dropTarget & goldAttack(Them, ksq);
            while (toBB) {
                const Square to = toBB.firstOneFromSQ11();
                if (attackersToIsAny(US, to)) {
                    if (!canKingEscape(*this, US, to, goldAttack(US, to))
                        && !canPieceCapture(*this, Them, to, dcBB_betweenIsThem))
                    {
                        return makeDropMove(Gold, to);
                    }
                }
            }
        }
        
        if (ourHand.exists<HSilver>()) {
            Bitboard toBB;
            if (ourHand.exists<HGold>()) {
                // 金打ちを先に調べたので、斜め後ろから打つ場合だけを調べる。
                
                if (ourHand.exists<HBishop>())
                    // 角打ちを先に調べたので、斜めからの王手も除外できる。銀打ちを調べる必要がない。
                    goto silver_drop_end;
                // 斜め後ろから打つ場合を調べる必要がある。
                toBB = dropTarget & (silverAttack(Them, ksq) & inFrontMask(US, makeRank(ksq)));
            }
            else {
                if (ourHand.exists<HBishop>())
                    // 斜め後ろを除外。前方から打つ場合を調べる必要がある。
                    toBB = dropTarget & goldAndSilverAttacks(Them, ksq);
                else
                    toBB = dropTarget & silverAttack(Them, ksq);
            }
            while (toBB) {
                const Square to = toBB.firstOneFromSQ11();
                if (attackersToIsAny(US, to)) {
                    if (!canKingEscape(*this, US, to, silverAttack(US, to))
                        && !canPieceCapture(*this, Them, to, dcBB_betweenIsThem))
                    {
                        return makeDropMove(Silver, to);
                    }
                }
            }
        }
    silver_drop_end:
        
        if (ourHand.exists<HKnight>()) {
            Bitboard toBB = dropTarget & knightAttack(Them, ksq);
            while (toBB) {
                const Square to = toBB.firstOneFromSQ11();
                // 桂馬は紐が付いている必要はない。
                // よって、このcanKingEscape() 内での to の位置に逃げられないようにする処理は無駄。
                if (!canKingEscape(*this, US, to, allZeroBB())
                    && !canPieceCapture(*this, Them, to, dcBB_betweenIsThem))
                {
                    return makeDropMove(Knight, to);
                }
            }
        }
        
        // 歩打ちで詰ますと反則なので、調べない。
        
        // 駒を移動する場合
        // moveTarget は桂馬以外の移動先の大まかな位置。飛角香の遠隔王手は含まない。
        const Bitboard moveTarget = bbOf(US).notThisAnd(kingAttack(ksq));
        const Bitboard pinned = pinnedBB();
        const Bitboard dcBB_betweenIsUs = discoveredCheckBB<true>();
        
        {
            // 竜による移動
            Bitboard fromBB = bbOf(Dragon, US);
            while (fromBB) {
                const Square from = fromBB.firstOneFromSQ11();
                // 遠隔王手は考えない。
                Bitboard toBB = moveTarget & attacksFrom<Dragon>(from);
                if (toBB) {
                    xorBBs(Dragon, from, US);
                    // 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
                    const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                    // to の位置の Bitboard は canKingEscape の中で更新する。
                    do {
                        const Square to = toBB.firstOneFromSQ11();
                        // 王手した駒の場所に自駒の利きがあるか。(無ければ玉で取られて詰まない)
                        if (unDropCheckIsSupported(US, to)) {
                            // 玉が逃げられない
                            // かつ、(空き王手 または 他の駒で取れない)
                            // かつ、王手した駒が pin されていない
                            if (!canKingEscape(*this, US, to, attacksFrom<Dragon>(to, occupiedBB() ^ setMaskBB(ksq)))
                                && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                    || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                            {
                                xorBBs(Dragon, from, US);
                                return makeCaptureMove(Dragon, from, to, *this);
                            }
                        }
                    } while (toBB);
                    xorBBs(Dragon, from, US);
                }
            }
        }
        
        // Txxx は先手、後手の情報を吸収した変数。数字は先手に合わせている。
        const Rank TRank4 = (US == Black ? Rank4 : Rank6);
        const Bitboard TRank123BB = inFrontMask<US, TRank4>();
        {
            // 飛車による移動
            Bitboard fromBB = bbOf(Rook, US);
            Bitboard fromOn123BB = fromBB & TRank123BB;
            // from が 123 段目
            if (fromOn123BB) {
                fromBB.andEqualNot(TRank123BB);
                do {
                    const Square from = fromOn123BB.firstOneFromSQ11();
                    Bitboard toBB = moveTarget & attacksFrom<Rook>(from);
                    if (toBB) {
                        xorBBs(Rook, from, US);
                        // 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
                        const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                        // to の位置の Bitboard は canKingEscape の中で更新する。
                        do {
                            const Square to = toBB.firstOneFromSQ11();
                            if (unDropCheckIsSupported(US, to)) {
                                if (!canKingEscape(*this, US, to, attacksFrom<Dragon>(to, occupiedBB() ^ setMaskBB(ksq)))
                                    && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                        || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                    && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                                {
                                    xorBBs(Rook, from, US);
                                    return makeCapturePromoteMove(Rook, from, to, *this);
                                }
                            }
                        } while (toBB);
                        xorBBs(Rook, from, US);
                    }
                } while (fromOn123BB);
            }
            
            // from が 4~9 段目
            while (fromBB) {
                const Square from = fromBB.firstOneFromSQ11();
                Bitboard toBB = moveTarget & attacksFrom<Rook>(from) & (rookStepAttacks(ksq) | TRank123BB);
                if (toBB) {
                    xorBBs(Rook, from, US);
                    // 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
                    const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                    
                    Bitboard toOn123BB = toBB & TRank123BB;
                    // 成り
                    if (toOn123BB) {
                        do {
                            const Square to = toOn123BB.firstOneFromSQ11();
                            if (unDropCheckIsSupported(US, to)) {
                                if (!canKingEscape(*this, US, to, attacksFrom<Dragon>(to, occupiedBB() ^ setMaskBB(ksq)))
                                    && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                        || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                    && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                                {
                                    xorBBs(Rook, from, US);
                                    return makeCapturePromoteMove(Rook, from, to, *this);
                                }
                            }
                        } while (toOn123BB);
                        
                        toBB.andEqualNot(TRank123BB);
                    }
                    // 不成
                    while (toBB) {
                        const Square to = toBB.firstOneFromSQ11();
                        if (unDropCheckIsSupported(US, to)) {
                            if (!canKingEscape(*this, US, to, rookAttackToEdge(to))
                                && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                    || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                            {
                                xorBBs(Rook, from, US);
                                return makeCaptureMove(Rook, from, to, *this);
                            }
                        }
                    }
                    xorBBs(Rook, from, US);
                }
            }
        }
        
        {
            // 馬による移動
            Bitboard fromBB = bbOf(Horse, US);
            while (fromBB) {
                const Square from = fromBB.firstOneFromSQ11();
                // 遠隔王手は考えない。
                Bitboard toBB = moveTarget & attacksFrom<Horse>(from);
                if (toBB) {
                    xorBBs(Horse, from, US);
                    // 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
                    const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                    // to の位置の Bitboard は canKingEscape の中で更新する。
                    do {
                        const Square to = toBB.firstOneFromSQ11();
                        // 王手した駒の場所に自駒の利きがあるか。(無ければ玉で取られて詰まない)
                        if (unDropCheckIsSupported(US, to)) {
                            // 玉が逃げられない
                            // かつ、(空き王手 または 他の駒で取れない)
                            // かつ、動かした駒が pin されていない)
                            if (!canKingEscape(*this, US, to, horseAttackToEdge(to)) // 竜の場合と違って、常に最大の利きを使用して良い。
                                && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                    || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                            {
                                xorBBs(Horse, from, US);
                                return makeCaptureMove(Horse, from, to, *this);
                            }
                        }
                    } while (toBB);
                    xorBBs(Horse, from, US);
                }
            }
        }
        
        {
            // 角による移動
            Bitboard fromBB = bbOf(Bishop, US);
            Bitboard fromOn123BB = fromBB & TRank123BB;
            // from が 123 段目
            if (fromOn123BB) {
                fromBB.andEqualNot(TRank123BB);
                do {
                    const Square from = fromOn123BB.firstOneFromSQ11();
                    Bitboard toBB = moveTarget & attacksFrom<Bishop>(from);
                    if (toBB) {
                        xorBBs(Bishop, from, US);
                        // 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
                        const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                        // to の位置の Bitboard は canKingEscape の中で更新する。
                        do {
                            const Square to = toBB.firstOneFromSQ11();
                            if (unDropCheckIsSupported(US, to)) {
                                if (!canKingEscape(*this, US, to, horseAttackToEdge(to))
                                    && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                        || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                    && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                                {
                                    xorBBs(Bishop, from, US);
                                    return makeCapturePromoteMove(Bishop, from, to, *this);
                                }
                            }
                        } while (toBB);
                        xorBBs(Bishop, from, US);
                    }
                } while (fromOn123BB);
            }
            
            // from が 4~9 段目
            while (fromBB) {
                const Square from = fromBB.firstOneFromSQ11();
                Bitboard toBB = moveTarget & attacksFrom<Bishop>(from) & (bishopStepAttacks(ksq) | TRank123BB);
                if (toBB) {
                    xorBBs(Bishop, from, US);
                    // 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
                    const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                    
                    Bitboard toOn123BB = toBB & TRank123BB;
                    // 成り
                    if (toOn123BB) {
                        do {
                            const Square to = toOn123BB.firstOneFromSQ11();
                            if (unDropCheckIsSupported(US, to)) {
                                if (!canKingEscape(*this, US, to, horseAttackToEdge(to))
                                    && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                        || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                    && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                                {
                                    xorBBs(Bishop, from, US);
                                    return makeCapturePromoteMove(Bishop, from, to, *this);
                                }
                            }
                        } while (toOn123BB);
                        
                        toBB.andEqualNot(TRank123BB);
                    }
                    // 不成
                    while (toBB) {
                        const Square to = toBB.firstOneFromSQ11();
                        if (unDropCheckIsSupported(US, to)) {
                            if (!canKingEscape(*this, US, to, bishopAttackToEdge(to))
                                && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                    || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                            {
                                xorBBs(Bishop, from, US);
                                return makeCaptureMove(Bishop, from, to, *this);
                            }
                        }
                    }
                    xorBBs(Bishop, from, US);
                }
            }
        }
        
        {
            // 金、成り金による移動
            Bitboard fromBB = goldsBB(US) & goldCheckTable(US, ksq);
            while (fromBB) {
                const Square from = fromBB.firstOneFromSQ11();
                Bitboard toBB = moveTarget & attacksFrom<Gold>(US, from) & attacksFrom<Gold>(Them, ksq);
                if (toBB) {
                    const PieceType pt = pieceToPieceType(piece(from));
                    xorBBs(pt, from, US);
                    goldsBB_.xorBit(from);
                    // 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
                    const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                    // to の位置の Bitboard は canKingEscape の中で更新する。
                    do {
                        const Square to = toBB.firstOneFromSQ11();
                        // 王手した駒の場所に自駒の利きがあるか。(無ければ玉で取られて詰まない)
                        if (unDropCheckIsSupported(US, to)) {
                            // 玉が逃げられない
                            // かつ、(空き王手 または 他の駒で取れない)
                            // かつ、動かした駒が pin されていない)
                            if (!canKingEscape(*this, US, to, attacksFrom<Gold>(US, to))
                                && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                    || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                            {
                                xorBBs(pt, from, US);
                                goldsBB_.xorBit(from);
                                return makeCaptureMove(pt, from, to, *this);
                            }
                        }
                    } while (toBB);
                    xorBBs(pt, from, US);
                    goldsBB_.xorBit(from);
                }
            }
        }
        
        {
            // 銀による移動
            Bitboard fromBB = bbOf(Silver, US) & silverCheckTable(US, ksq);
            if (fromBB) {
                // Txxx は先手、後手の情報を吸収した変数。数字は先手に合わせている。
                const Bitboard TRank5_9BB = inFrontMask<Them, TRank4>();
                const Bitboard chkBB = attacksFrom<Silver>(Them, ksq);
                const Bitboard chkBB_promo = attacksFrom<Gold>(Them, ksq);
                
                Bitboard fromOn123BB = fromBB & TRank123BB;
                // from が敵陣
                if (fromOn123BB) {
                    fromBB.andEqualNot(TRank123BB);
                    do {
                        const Square from = fromOn123BB.firstOneFromSQ11();
                        Bitboard toBB = moveTarget & attacksFrom<Silver>(US, from);
                        Bitboard toBB_promo = toBB & chkBB_promo;
                        
                        toBB &= chkBB;
                        if ((toBB_promo | toBB)) {
                            xorBBs(Silver, from, US);
                            // 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
                            const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                            // to の位置の Bitboard は canKingEscape の中で更新する。
                            while (toBB_promo) {
                                const Square to = toBB_promo.firstOneFromSQ11();
                                if (unDropCheckIsSupported(US, to)) {
                                    // 成り
                                    if (!canKingEscape(*this, US, to, attacksFrom<Gold>(US, to))
                                        && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                            || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                        && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                                    {
                                        xorBBs(Silver, from, US);
                                        return makeCapturePromoteMove(Silver, from, to, *this);
                                    }
                                }
                            }
                            
                            // 玉の前方に移動する場合、成で詰まなかったら不成でも詰まないので、ここで省く。
                            // sakurapyon の作者が言ってたので実装。
                            toBB.andEqualNot(inFrontMask(Them, makeRank(ksq)));
                            while (toBB) {
                                const Square to = toBB.firstOneFromSQ11();
                                if (unDropCheckIsSupported(US, to)) {
                                    // 不成
                                    if (!canKingEscape(*this, US, to, attacksFrom<Silver>(US, to))
                                        && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                            || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                        && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                                    {
                                        xorBBs(Silver, from, US);
                                        return makeCaptureMove(Silver, from, to, *this);
                                    }
                                }
                            }
                            
                            xorBBs(Silver, from, US);
                        }
                    } while (fromOn123BB);
                }
                
                // from が 5~9段目 (必ず不成)
                Bitboard fromOn5_9BB = fromBB & TRank5_9BB;
                if (fromOn5_9BB) {
                    fromBB.andEqualNot(TRank5_9BB);
                    do {
                        const Square from = fromOn5_9BB.firstOneFromSQ11();
                        Bitboard toBB = moveTarget & attacksFrom<Silver>(US, from) & chkBB;
                        
                        if (toBB) {
                            xorBBs(Silver, from, US);
                            // 動いた後の dcBB, pinned: to の位置の occupied や checkers は関係ないので、ここで生成できる。
                            const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                            // to の位置の Bitboard は canKingEscape の中で更新する。
                            while (toBB) {
                                const Square to = toBB.firstOneFromSQ11();
                                if (unDropCheckIsSupported(US, to)) {
                                    // 不成
                                    if (!canKingEscape(*this, US, to, attacksFrom<Silver>(US, to))
                                        && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                            || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                        && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                                    {
                                        xorBBs(Silver, from, US);
                                        return makeCaptureMove(Silver, from, to, *this);
                                    }
                                }
                            }
                            
                            xorBBs(Silver, from, US);
                        }
                    } while (fromOn5_9BB);
                }
                
                // 残り 4 段目のみ
                // 前進するときは成れるが、後退するときは成れない。
                while (fromBB) {
                    const Square from = fromBB.firstOneFromSQ11();
                    Bitboard toBB = moveTarget & attacksFrom<Silver>(US, from);
                    Bitboard toBB_promo = toBB & TRank123BB & chkBB_promo; // 3 段目にしか成れない。
                    
                    toBB &= chkBB;
                    if ((toBB_promo | toBB)) {
                        xorBBs(Silver, from, US);
                        // 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
                        const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                        // to の位置の Bitboard は canKingEscape の中で更新する。
                        while (toBB_promo) {
                            const Square to = toBB_promo.firstOneFromSQ11();
                            if (unDropCheckIsSupported(US, to)) {
                                // 成り
                                if (!canKingEscape(*this, US, to, attacksFrom<Gold>(US, to))
                                    && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                        || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                    && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                                {
                                    xorBBs(Silver, from, US);
                                    return makeCapturePromoteMove(Silver, from, to, *this);
                                }
                            }
                        }
                        
                        while (toBB) {
                            const Square to = toBB.firstOneFromSQ11();
                            if (unDropCheckIsSupported(US, to)) {
                                // 不成
                                if (!canKingEscape(*this, US, to, attacksFrom<Silver>(US, to))
                                    && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                        || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                    && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                                {
                                    xorBBs(Silver, from, US);
                                    return makeCaptureMove(Silver, from, to, *this);
                                }
                            }
                        }
                        
                        xorBBs(Silver, from, US);
                    }
                }
            }
        }
        
        {
            // 桂による移動
            Bitboard fromBB = bbOf(Knight, US) & knightCheckTable(US, ksq);
            if (fromBB) {
                const Bitboard chkBB_promo = attacksFrom<Gold>(Them, ksq) & TRank123BB;
                const Bitboard chkBB = attacksFrom<Knight>(Them, ksq);
                
                do {
                    const Square from = fromBB.firstOneFromSQ11();
                    Bitboard toBB = bbOf(US).notThisAnd(attacksFrom<Knight>(US, from));
                    Bitboard toBB_promo = toBB & chkBB_promo;
                    toBB &= chkBB;
                    if ((toBB_promo | toBB)) {
                        xorBBs(Knight, from, US);
                        // 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
                        const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                        // to の位置の Bitboard は canKingEscape の中で更新する。
                        while (toBB_promo) {
                            const Square to = toBB_promo.firstOneFromSQ11();
                            if (unDropCheckIsSupported(US, to)) {
                                // 成り
                                if (!canKingEscape(*this, US, to, attacksFrom<Gold>(US, to))
                                    && (isDiscoveredCheck<true>(from, to, ksq, dcBB_betweenIsUs)
                                        || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                    && !isPinnedIllegal<true>(from, to, kingSquare(US), pinned))
                                {
                                    xorBBs(Knight, from, US);
                                    return makeCapturePromoteMove(Knight, from, to, *this);
                                }
                            }
                        }
                        
                        while (toBB) {
                            const Square to = toBB.firstOneFromSQ11();
                            // 桂馬は紐が付いてなくて良いので、紐が付いているかは調べない。
                            // 不成
                            if (!canKingEscape(*this, US, to, allZeroBB())
                                && (isDiscoveredCheck<true>(from, to, ksq, dcBB_betweenIsUs)
                                    || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                && !isPinnedIllegal<true>(from, to, kingSquare(US), pinned))
                            {
                                xorBBs(Knight, from, US);
                                return makeCaptureMove(Knight, from, to, *this);
                            }
                        }
                        xorBBs(Knight, from, US);
                    }
                } while (fromBB);
            }
        }
        
        {
            // 香車による移動
            Bitboard fromBB = bbOf(Lance, US) & lanceCheckTable(US, ksq);
            if (fromBB) {
                // Txxx は先手、後手の情報を吸収した変数。数字は先手に合わせている。
                const SquareDelta TDeltaS = (US == Black ? DeltaS : DeltaN);
                const Rank TRank2 = (US == Black ? Rank2 : Rank8);
                const Bitboard chkBB_promo = attacksFrom<Gold>(Them, ksq) & TRank123BB;
                // 玉の前方1マスのみ。
                // 玉が 1 段目にいるときは、成のみで良いので省く。
                const Bitboard chkBB = attacksFrom<Pawn>(Them, ksq) & inFrontMask<Them, TRank2>();
                
                do {
                    const Square from = fromBB.firstOneFromSQ11();
                    Bitboard toBB = moveTarget & attacksFrom<Lance>(US, from);
                    Bitboard toBB_promo = toBB & chkBB_promo;
                    
                    toBB &= chkBB;
                    
                    if ((toBB_promo | toBB)) {
                        xorBBs(Lance, from, US);
                        // 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
                        const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                        // to の位置の Bitboard は canKingEscape の中で更新する。
                        
                        while (toBB_promo) {
                            const Square to = toBB_promo.firstOneFromSQ11();
                            if (unDropCheckIsSupported(US, to)) {
                                // 成り
                                if (!canKingEscape(*this, US, to, attacksFrom<Gold>(US, to))
                                    && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                        || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                    && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                                {
                                    xorBBs(Lance, from, US);
                                    return makeCapturePromoteMove(Lance, from, to, *this);
                                }
                            }
                        }
                        
                        if (toBB) {
                            // assert(toBB.isOneBit());
                            if(!toBB.isOneBit()){
                                printf("error, assert(toBB.isOneBit())\n");
                            }
                            // 不成で王手出来るのは、一つの場所だけなので、ループにする必要が無い。
                            const Square to = ksq + TDeltaS;
                            if (unDropCheckIsSupported(US, to)) {
                                // 不成
                                if (!canKingEscape(*this, US, to, lanceAttackToEdge(US, to))
                                    && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                        || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                    && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                                {
                                    xorBBs(Lance, from, US);
                                    return makeCaptureMove(Lance, from, to, *this);
                                }
                            }
                        }
                        xorBBs(Lance, from, US);
                    }
                } while (fromBB);
            }
        }
        
        {
            // 歩による移動
            // 成れる場合は必ずなる。
            // todo: PawnCheckBB 作って簡略化する。
            const Rank krank = makeRank(ksq);
            // 歩が移動して王手になるのは、相手玉が1~7段目の時のみ。
            if (isInFrontOf<US, Rank8, Rank2>(krank)) {
                // Txxx は先手、後手の情報を吸収した変数。数字は先手に合わせている。
                const SquareDelta TDeltaS = (US == Black ? DeltaS : DeltaN);
                const SquareDelta TDeltaN = (US == Black ? DeltaN : DeltaS);
                
                Bitboard fromBB = bbOf(Pawn, US);
                // 玉が敵陣にいないと成で王手になることはない。
                if (isInFrontOf<US, Rank4, Rank6>(krank)) {
                    // 成った時に王手になる位置
                    const Bitboard toBB_promo = moveTarget & attacksFrom<Gold>(Them, ksq) & TRank123BB;
                    Bitboard fromBB_promo = fromBB & pawnAttack<Them>(toBB_promo);
                    while (fromBB_promo) {
                        const Square from = fromBB_promo.firstOneFromSQ11();
                        const Square to = from + TDeltaN;
                        
                        xorBBs(Pawn, from, US);
                        // 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
                        const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                        // to の位置の Bitboard は canKingEscape の中で更新する。
                        if (unDropCheckIsSupported(US, to)) {
                            // 成り
                            if (!canKingEscape(*this, US, to, attacksFrom<Gold>(US, to))
                                && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                    || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                            {
                                xorBBs(Pawn, from, US);
                                return makeCapturePromoteMove(Pawn, from, to, *this);
                            }
                        }
                        xorBBs(Pawn, from, US);
                    }
                }
                
                // 不成
                // 玉が 8,9 段目にいることは無いので、from,to が隣の筋を指すことは無い。
                const Square to = ksq + TDeltaS;
                const Square from = to + TDeltaS;
                if (fromBB.isSet(from) && !bbOf(US).isSet(to)) {
                    // 玉が 1, 2 段目にいるなら、成りで王手出来るので不成は調べない。
                    if (isBehind<US, Rank2, Rank8>(krank)) {
                        xorBBs(Pawn, from, US);
                        // 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
                        const Bitboard dcBB_betweenIsThem_after = discoveredCheckBB<false>();
                        // to の位置の Bitboard は canKingEscape の中で更新する。
                        if (unDropCheckIsSupported(US, to)) {
                            // 不成
                            if (!canKingEscape(*this, US, to, allZeroBB())
                                && (isDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
                                    || !canPieceCapture(*this, Them, to, dcBB_betweenIsThem_after))
                                && !isPinnedIllegal(from, to, kingSquare(US), pinned))
                            {
                                xorBBs(Pawn, from, US);
                                return makeCaptureMove(Pawn, from, to, *this);
                            }
                        }
                        xorBBs(Pawn, from, US);
                    }
                }
            }
        }
        
        return Move::moveNone();
    }
    
    Move Position::mateMoveIn1Ply() {
        return (turn() == Black ? mateMoveIn1Ply<Black>() : mateMoveIn1Ply<White>());
    }
    
    void Position::initZobrist() {
        // zobTurn_ は 1 であり、その他は 1桁目を使わない。
        // zobTurn のみ xor で更新する為、他の桁に影響しないようにする為。
        // hash値の更新は普通は全て xor を使うが、持ち駒の更新の為に +, - を使用した方が都合が良い。
        for (PieceType pt = Occupied; pt < PieceTypeNum; ++pt) {
            for (Square sq = SQ11; sq < SquareNum; ++sq) {
                for (Color c = Black; c < ColorNum; ++c){
                    u64 random = g_mt64bit.random();
                    // printf("g_mt64bit random: %llu\n", random);
                    zobrist_[pt][sq][c] = random & ~UINT64_C(1);
                    // printf("%llu\n", zobrist_[pt][sq][c]);
                }
            }
        }
        for (HandPiece hp = HPawn; hp < HandPieceNum; ++hp) {
            zobHand_[hp][Black] = g_mt64bit.random() & ~UINT64_C(1);
            zobHand_[hp][White] = g_mt64bit.random() & ~UINT64_C(1);
        }
        zobExclusion_ = g_mt64bit.random() & ~UINT64_C(1);
        
        /*int32_t random = 0;
        
        for (PieceType pt = Occupied; pt < PieceTypeNum; ++pt) {
            for (Square sq = SQ11; sq < SquareNum; ++sq) {
                for (Color c = Black; c < ColorNum; ++c){
                    zobrist_[pt][sq][c] = (random++) & ~UINT64_C(1);
                }
            }
        }
        for (HandPiece hp = HPawn; hp < HandPieceNum; ++hp) {
            zobHand_[hp][Black] = (random++) & ~UINT64_C(1);
            zobHand_[hp][White] = (random++) & ~UINT64_C(1);
        }
        zobExclusion_ = random++;*/
    }
    
    void Position::print() const {
        std::cout << "'  9  8  7  6  5  4  3  2  1" << std::endl;
        int32_t i = 0;
        for (Rank r = Rank1; r < RankNum; ++r) {
            ++i;
            std::cout << "P" << i;
            for (File f = File9; File1 <= f; --f)
                std::cout << pieceToCharCSA(piece(makeSquare(f, r)));
            std::cout << std::endl;
        }
        printHand(Black);
        printHand(White);
        std::cout << (turn() == Black ? "+" : "-") << std::endl;
        std::cout << std::endl;
        std::cout << "key = " << getKey() << std::endl;
    }
    
    // TODO dung stringstream se co van de voi nhieu thread
    std::string Position::toSFEN(const Ply ply) const {
        std::stringstream ss;
        // ss << "sfen ";
        int32_t space = 0;
        for (Rank rank = Rank1; rank <= Rank9; ++rank) {
            for (File file = File9; file >= File1; --file) {
                const Square sq = makeSquare(file, rank);
                const Piece pc = piece(sq);
                if (pc == Empty)
                    ++space;
                else {
                    if (space) {
                        ss << space;
                        space = 0;
                    }
                    ss << pieceToCharUSI(pc);
                }
            }
            if (space) {
                ss << space;
                space = 0;
            }
            if (rank != Rank9)
                ss << "/";
        }
        ss << (turn() == Black ? " b " : " w ");
        if (hand(Black).value() == 0 && hand(White).value() == 0)
            ss << "- ";
        else {
            // USI の規格として、持ち駒の表記順は決まっており、先手、後手の順で、それぞれ 飛、角、金、銀、桂、香、歩 の順。
            for (Color color = Black; color < ColorNum; ++color) {
                for (HandPiece hp : {HRook, HBishop, HGold, HSilver, HKnight, HLance, HPawn}) {
                    const int32_t num = hand(color).numOf(hp);
                    if (num == 0)
                        continue;
                    if (num != 1)
                        ss << num;
                    const Piece pc = colorAndHandPieceToPiece(color, hp);
                    ss << pieceToCharUSI(pc);
                }
            }
            ss << " ";
        }
        ss << ply;
        return ss.str();
    }
    
    const int32_t ToSfenMaxLength = 1000;
    void Position::myToSFEN(const Ply ply, char* ss) const {
        {
            ss[0] = 0;
        }
        {
            int32_t space = 0;
            for (Rank rank = Rank1; rank <= Rank9; ++rank) {
                for (File file = File9; file >= File1; --file) {
                    const Square sq = makeSquare(file, rank);
                    const Piece pc = piece(sq);
                    if (pc == Empty)
                        ++space;
                    else {
                        if (space) {
                            snprintf(ss, ToSfenMaxLength, "%s%d", ss, space);
                            space = 0;
                        }
                        snprintf(ss, ToSfenMaxLength, "%s%s", ss, pieceToCharUSI(pc));
                    }
                }
                if (space) {
                    snprintf(ss, ToSfenMaxLength, "%s%d", ss, space);
                    space = 0;
                }
                if (rank != Rank9)
                    snprintf(ss, ToSfenMaxLength, "%s/", ss);
            }
            snprintf(ss, ToSfenMaxLength, "%s%s", ss, (turn() == Black ? " b " : " w "));
            if (hand(Black).value() == 0 && hand(White).value() == 0)
                snprintf(ss, ToSfenMaxLength, "%s- ", ss);
            else {
                // USI の規格として、持ち駒の表記順は決まっており、先手、後手の順で、それぞれ 飛、角、金、銀、桂、香、歩 の順。
                for (Color color = Black; color < ColorNum; ++color) {
                    for (HandPiece hp : {HRook, HBishop, HGold, HSilver, HKnight, HLance, HPawn}) {
                        const int32_t num = hand(color).numOf(hp);
                        if (num == 0)
                            continue;
                        if (num != 1)
                            snprintf(ss, ToSfenMaxLength, "%s%d", ss, num);
                        const Piece pc = colorAndHandPieceToPiece(color, hp);
                        snprintf(ss, ToSfenMaxLength, "%s%s", ss, pieceToCharUSI(pc));
                    }
                }
                snprintf(ss, ToSfenMaxLength, "%s ", ss);
            }
            snprintf(ss, ToSfenMaxLength, "%s%d", ss, ply);
        }
    }
    
    HuffmanCodedPos Position::toHuffmanCodedPos() const {
        HuffmanCodedPos result;
        result.clear();
        BitStream bs(result.data);
        // 手番 (1bit)
        bs.putBit(turn());
        
        // 玉の位置 (7bit * 2)
        bs.putBits(kingSquare(Black), 7);
        bs.putBits(kingSquare(White), 7);
        
        // 盤上の駒
        for (Square sq = SQ11; sq < SquareNum; ++sq) {
            Piece pc = piece(sq);
            if (pieceToPieceType(pc) == King)
                continue;
            const auto hc = HuffmanCodedPos::boardCodeTable[pc];
            bs.putBits(hc.code, hc.numOfBits);
        }
        
        // 持ち駒
        for (Color c = Black; c < ColorNum; ++c) {
            const Hand h = hand(c);
            for (HandPiece hp = HPawn; hp < HandPieceNum; ++hp) {
                const auto hc = HuffmanCodedPos::handCodeTable[hp][c];
                for (u32 n = 0; n < h.numOf(hp); ++n)
                    bs.putBits(hc.code, hc.numOfBits);
            }
        }
        // assert(bs.data() == std::end(result.data));
        if(!(bs.data() == std::end(result.data))){
            printf("error, assert(bs.data() == std::end(result.data))\n");
        }
        // assert(bs.curr() == 0);
        if(!(bs.curr() == 0)){
            printf("error, assert(bs.curr() == 0)\n");
        }
        return result;
    }

    bool Position::isOK() const {
        static Key prevKey;
        const bool debugAll = true;
        
        const bool debugBitboards    = debugAll || false;
        const bool debugKingCount    = debugAll || false;
        const bool debugKingCapture  = debugAll || false;
        const bool debugCheckerCount = debugAll || false;
        const bool debugKey          = debugAll || false;
        const bool debugStateHand    = debugAll || false;
        const bool debugPiece        = debugAll || false;
        const bool debugMaterial     = debugAll || false;

        int32_t failedStep = 0;
        // stateInfo
        if(this->st_==NULL){
            goto incorrect_position;
        }
        
        // bitboard
        ++failedStep;
        if (debugBitboards) {
            if ((bbOf(Black) & bbOf(White)))
                goto incorrect_position;
            if ((bbOf(Black) | bbOf(White)) != occupiedBB())
                goto incorrect_position;
            if ((bbOf(Pawn     ) ^ bbOf(Lance    ) ^ bbOf(Knight) ^ bbOf(Silver ) ^ bbOf(Bishop  ) ^
                 bbOf(Rook     ) ^ bbOf(Gold     ) ^ bbOf(King  ) ^ bbOf(ProPawn) ^ bbOf(ProLance) ^
                 bbOf(ProKnight) ^ bbOf(ProSilver) ^ bbOf(Horse ) ^ bbOf(Dragon )) != occupiedBB())
            {
                goto incorrect_position;
            }
            for (PieceType pt1 = Pawn; pt1 < PieceTypeNum; ++pt1) {
                for (PieceType pt2 = pt1 + 1; pt2 < PieceTypeNum; ++pt2) {
                    if ((bbOf(pt1) & bbOf(pt2)))
                        goto incorrect_position;
                }
            }
        }
        
        // kingCount
        ++failedStep;
        if (debugKingCount) {
            int32_t kingCount[ColorNum] = {0, 0};
            if (bbOf(King).popCount() != 2)
                goto incorrect_position;
            if (!bbOf(King, Black).isOneBit())
                goto incorrect_position;
            if (!bbOf(King, White).isOneBit())
                goto incorrect_position;
            for (Square sq = SQ11; sq < SquareNum; ++sq) {
                if (piece(sq) == BKing)
                    ++kingCount[Black];
                if (piece(sq) == WKing)
                    ++kingCount[White];
            }
            if (kingCount[Black] != 1 || kingCount[White] != 1)
                goto incorrect_position;
        }
        
        // capture
        ++failedStep;
        if (debugKingCapture) {
            // 相手玉を取れないことを確認
            const Color us = turn();
            const Color them = oppositeColor(us);
            const Square ksq = kingSquare(them);
            if (attackersTo(us, ksq))
                goto incorrect_position;
        }
        
        // checkerCount
        ++failedStep;
        if (debugCheckerCount) {
            if (2 < st_->checkersBB.popCount())
                goto incorrect_position;
        }
        
        // key
        ++failedStep;
        if (debugKey) {
            if (getKey() != computeKey())
                goto incorrect_position;
        }
        
        // hand
        ++failedStep;
        if (debugStateHand) {
            if (st_->hand != hand(turn()))
                goto incorrect_position;
        }
        
        // piece
        ++failedStep;
        if (debugPiece) {
            for (Square sq = SQ11; sq < SquareNum; ++sq) {
                const Piece pc = piece(sq);
                if (pc == Empty) {
                    if (!emptyBB().isSet(sq))
                        goto incorrect_position;
                }
                else {
                    if (!bbOf(pieceToPieceType(pc), pieceToColor(pc)).isSet(sq))
                        goto incorrect_position;
                }
            }
        }
        
        // material
        ++failedStep;
        if (debugMaterial) {
            if (material() != computeMaterial())
                goto incorrect_position;
        }
        
        // evalList
        ++failedStep;
        {
            int32_t i;
            if ((i = debugSetEvalList()) != 0) {
                std::cout << "debugSetEvalList() error = " << i << std::endl;
                goto incorrect_position;
            }
        }
        
        prevKey = getKey();
        return true;
        
    incorrect_position:
        // std::cout << "Error! failedStep = " << failedStep << std::endl;
        // std::cout << "prevKey = " << prevKey << std::endl;
        // std::cout << "currKey = " << getKey() << std::endl;
        // printf("Error! failedStep = %d\n", failedStep);
        // print();
        return false;
    }

    int32_t Position::myIsOK() const {
        static Key prevKey;
        const bool debugAll = true;
        
        const bool debugBitboards    = debugAll || false;
        const bool debugKingCount    = debugAll || false;
        const bool debugKingCapture  = debugAll || false;
        const bool debugCheckerCount = debugAll || false;
        const bool debugKey          = debugAll || false;
        const bool debugStateHand    = debugAll || false;
        const bool debugPiece        = debugAll || false;
        const bool debugMaterial     = debugAll || false;

        int32_t failedStep = 0;
        if (debugBitboards) {
            if ((bbOf(Black) & bbOf(White)))
                goto incorrect_position;
            if ((bbOf(Black) | bbOf(White)) != occupiedBB())
                goto incorrect_position;
            if ((bbOf(Pawn     ) ^ bbOf(Lance    ) ^ bbOf(Knight) ^ bbOf(Silver ) ^ bbOf(Bishop  ) ^
                 bbOf(Rook     ) ^ bbOf(Gold     ) ^ bbOf(King  ) ^ bbOf(ProPawn) ^ bbOf(ProLance) ^
                 bbOf(ProKnight) ^ bbOf(ProSilver) ^ bbOf(Horse ) ^ bbOf(Dragon )) != occupiedBB())
            {
                goto incorrect_position;
            }
            for (PieceType pt1 = Pawn; pt1 < PieceTypeNum; ++pt1) {
                for (PieceType pt2 = pt1 + 1; pt2 < PieceTypeNum; ++pt2) {
                    if ((bbOf(pt1) & bbOf(pt2)))
                        goto incorrect_position;
                }
            }
        }
        
        ++failedStep;// 1
        if (debugKingCount) {
            int32_t kingCount[ColorNum] = {0, 0};
            if (bbOf(King).popCount() != 2)
                goto incorrect_position;
            if (!bbOf(King, Black).isOneBit())
                goto incorrect_position;
            if (!bbOf(King, White).isOneBit())
                goto incorrect_position;
            for (Square sq = SQ11; sq < SquareNum; ++sq) {
                if (piece(sq) == BKing)
                    ++kingCount[Black];
                if (piece(sq) == WKing)
                    ++kingCount[White];
            }
            if (kingCount[Black] != 1 || kingCount[White] != 1)
                goto incorrect_position;
        }
        
        ++failedStep;// 2
        if (debugKingCapture) {
            // 相手玉を取れないことを確認
            const Color us = turn();
            const Color them = oppositeColor(us);
            const Square ksq = kingSquare(them);
            if (attackersTo(us, ksq))
                goto incorrect_position;
        }
        
        ++failedStep;// 3
        if (debugCheckerCount) {
            if (2 < st_->checkersBB.popCount())
                goto incorrect_position;
        }
        
        ++failedStep;// 4
        if (debugKey) {
            if (getKey() != computeKey())
                goto incorrect_position;
        }
        
        ++failedStep;
        if (debugStateHand) {
            if (st_->hand != hand(turn()))
                goto incorrect_position;
        }
        
        ++failedStep;
        if (debugPiece) {
            for (Square sq = SQ11; sq < SquareNum; ++sq) {
                const Piece pc = piece(sq);
                if (pc == Empty) {
                    if (!emptyBB().isSet(sq))
                        goto incorrect_position;
                }
                else {
                    if (!bbOf(pieceToPieceType(pc), pieceToColor(pc)).isSet(sq))
                        goto incorrect_position;
                }
            }
        }
        
        ++failedStep;
        if (debugMaterial) {
            if (material() != computeMaterial())
                goto incorrect_position;
        }
        
        ++failedStep;
        {
            int32_t i;
            if ((i = debugSetEvalList()) != 0) {
                std::cout << "debugSetEvalList() error = " << i << std::endl;
                goto incorrect_position;
            }
        }
        
        // evaluate
        ++failedStep;
        {
            if(Evaluator::allocated){
                const Square sq_bk = this->kingSquare(Black);
                const Square sq_wk = this->kingSquare(White);
                const EvalIndex* list0 = &evalList_.list0[0]; ;
                const EvalIndex* list1 = &evalList_.list1[0];
                
                const auto* ppkppb = Evaluator::KPP[sq_bk         ];
                const auto* ppkppw = Evaluator::KPP[inverse(sq_wk)];
                
                EvalSum sum;
                sum.p[2][0] = 0;
                sum.p[2][1] = 0;
#if defined USE_AVX2_EVAL || defined USE_SSE_EVAL
                sum.m[0] = _mm_setzero_si128();
                for (int32_t i = 0; i < pos.nlist(); ++i) {
                    const int32_t k0 = list0[i];
                    const int32_t k1 = list1[i];
                    const auto* pkppb = ppkppb[k0];
                    const auto* pkppw = ppkppw[k1];
                    for (int32_t j = 0; j < i; ++j) {
                        const int32_t l0 = list0[j];
                        const int32_t l1 = list1[j];
                        __m128i tmp;
                        tmp = _mm_set_epi32(0, 0, *reinterpret_cast<const s32*>(&pkppw[l1][0]), *reinterpret_cast<const s32*>(&pkppb[l0][0]));
                        tmp = _mm_cvtepi16_epi32(tmp);
                        sum.m[0] = _mm_add_epi32(sum.m[0], tmp);
                    }
                    sum.p[2] += Evaluator::allocated ? Evaluator::KKP[sq_bk][sq_wk][k0] : 0;
                }
#else
                sum.p[0][0] = 0;
                sum.p[0][1] = 0;
                sum.p[1][0] = 0;
                sum.p[1][1] = 0;
                for (int32_t i = 0; i < this->nlist(); ++i) {
                    const int32_t k0 = list0[i];
                    const int32_t k1 = list1[i];
                    const auto* pkppb = ppkppb[k0];
                    const auto* pkppw = ppkppw[k1];
                    for (int32_t j = 0; j < i; ++j) {
                        const int32_t l0 = list0[j];
                        const int32_t l1 = list1[j];
                        sum.p[0] += pkppb[l0];
                        sum.p[1] += pkppw[l1];
                    }
                    if(Evaluator::allocated){
                        sum.p[2] += Evaluator::KKP[sq_bk][sq_wk][k0];
                    } else {
                        printf("Evaluator not allocate");
                    }
                }
#endif
                
                sum.p[2][0] += this->material() * FVScale;
#if defined INANIWA_SHIFT
                sum.p[2][0] += inaniwaScore(pos);
#endif
                
                if(evaluateUnUseDiff(*this) != sum.sum(this->turn())){
#ifdef Android
                    __android_log_print(ANDROID_LOG_ERROR, "NativeCore", "error, myPosition evaluate error\n");
#else
                    printf("error, myPosition evaluate error\n");
#endif
                    goto incorrect_position;
                }
            } else {
                // printf("error, evaluator not allocated\n");
            }
        }
        
        prevKey = getKey();
        return -1;
        
    incorrect_position:
        // std::cout << "Error! failedStep = " << failedStep << std::endl;
        // std::cout << "prevKey = " << prevKey << std::endl;
        // std::cout << "currKey = " << getKey() << std::endl;
        // print();
#ifdef Android
        __android_log_print(ANDROID_LOG_ERROR, "NativeCore", "error, failStep: %d\n", failedStep);
#else
        printf("error, failStep: %d\n", failedStep);
#endif
        return failedStep;
    }

    int32_t Position::debugSetEvalList() const {
        // not implement
        return 0;
    }
    
    Key Position::computeBoardKey() const {
        Key result = 0;
        for (Square sq = SQ11; sq < SquareNum; ++sq) {
            if (piece(sq) != Empty)
                result += zobrist(pieceToPieceType(piece(sq)), sq, pieceToColor(piece(sq)));
        }
        if (turn() == White)
            result ^= zobTurn();
        return result;
    }
    
    Key Position::computeHandKey() const {
        Key result = 0;
        for (HandPiece hp = HPawn; hp < HandPieceNum; ++hp) {
            for (Color c = Black; c < ColorNum; ++c) {
                const int32_t num = hand(c).numOf(hp);
                for (int32_t i = 0; i < num; ++i)
                    result += zobHand(hp, c);
            }
        }
        return result;
    }
    
    // todo: isRepetition() に名前変えた方が良さそう。
    //       同一局面4回をきちんと数えていないけど問題ないか。
    RepetitionType Position::isDraw(const int32_t checkMaxPly) const {
        const int32_t Start = 4;
        int32_t i = Start;
        const int32_t e = min(st_->pliesFromNull, checkMaxPly);
        
        // 4手掛けないと千日手には絶対にならない。
        if (i <= e) {
            // 現在の局面と、少なくとも 4 手戻らないと同じ局面にならない。
            // ここでまず 2 手戻る。
            StateInfo* stp = st_->previous->previous;
            
            do {
                // 更に 2 手戻る。
                if(stp->previous!=NULL){
                    stp = stp->previous->previous;
                }else{
                    printf("isDraw: why stp previous null\n");
                }
                // if (stp->key() == st_->key()) {
                if ((stp!=NULL && st_!=NULL) && (stp->key() == st_->key())) {
                    if (i <= st_->continuousCheck[turn()])
                        return RepetitionLose;
                    else if (i <= st_->continuousCheck[oppositeColor(turn())])
                        return RepetitionWin;
#if defined BAN_BLACK_REPETITION
                    return (turn() == Black ? RepetitionLose : RepetitionWin);
#elif defined BAN_WHITE_REPETITION
                    return (turn() == White ? RepetitionLose : RepetitionWin);
#else
                    return RepetitionDraw;
#endif
                }
                // else if (stp->boardKey == st_->boardKey) {
                else if ((stp!=NULL && st_!=NULL) && (stp->boardKey == st_->boardKey)) {
                    if (st_->hand.isEqualOrSuperior(stp->hand)) return RepetitionSuperior;
                    if (stp->hand.isEqualOrSuperior(st_->hand)) return RepetitionInferior;
                }
                i += 2;
            } while (i <= e);
        }
        return NotRepetition;
    }
    
    namespace {
        void printHandPiece(const Position& pos, const HandPiece hp, const Color c, const std::string& str) {
            if (pos.hand(c).numOf(hp)) {
                const char* sign = (c == Black ? "+" : "-");
                std::cout << "P" << sign;
                for (u32 i = 0; i < pos.hand(c).numOf(hp); ++i)
                    std::cout << "00" << str;
                std::cout << std::endl;
            }
        }
    }
    void Position::printHand(const Color c) const {
        printHandPiece(*this, HPawn  , c, "FU");
        printHandPiece(*this, HLance , c, "KY");
        printHandPiece(*this, HKnight, c, "KE");
        printHandPiece(*this, HSilver, c, "GI");
        printHandPiece(*this, HGold  , c, "KI");
        printHandPiece(*this, HBishop, c, "KA");
        printHandPiece(*this, HRook  , c, "HI");
    }
    
    Position& Position::operator = (const Position& pos) {
        memcpy(this, &pos, sizeof(Position));
        startState_ = *st_;
        st_ = &startState_;
        nodes_ = 0;
        
        // assert(isOK());
        if(!isOK()){
            printf("error, assert(isOK())\n");
        }
        return *this;
    }
    
    void Position::set(const std::string& sfen, Thread* th) {
        Piece promoteFlag = UnPromoted;
        std::istringstream ss(sfen);
        char token;
        Square sq = SQ91;
        
        Searcher* s = std::move(searcher_);
        clear();
        setSearcher(s);
        
        // 盤上の駒
        while (ss.get(token) && token != ' ') {
            if (isdigit(token))
                sq += DeltaE * (token - '0');
            else if (token == '/')
                sq += (DeltaW * 9) + DeltaS;
            else if (token == '+')
                promoteFlag = Promoted;
            else if (g_charToPieceUSI.isLegalChar(token)) {
                if (isInSquare(sq)) {
                    setPiece(g_charToPieceUSI.value(token) + promoteFlag, sq);
                    promoteFlag = UnPromoted;
                    sq += DeltaE;
                }
                else
                    goto INCORRECT;
            }
            else
                goto INCORRECT;
        }
        kingSquare_[Black] = bbOf(King, Black).constFirstOneFromSQ11();
        kingSquare_[White] = bbOf(King, White).constFirstOneFromSQ11();
        goldsBB_ = bbOf(Gold, ProPawn, ProLance, ProKnight, ProSilver);
        
        // 手番
        while (ss.get(token) && token != ' ') {
            if (token == 'b')
                turn_ = Black;
            else if (token == 'w')
                turn_ = White;
            else
                goto INCORRECT;
        }
        
        // 持ち駒
        for (int32_t digits = 0; ss.get(token) && token != ' '; ) {
            if (token == '-')
                memset(hand_, 0, sizeof(hand_));
            else if (isdigit(token))
                digits = digits * 10 + token - '0';
            else if (g_charToPieceUSI.isLegalChar(token)) {
                // 持ち駒を32bit に pack する
                const Piece piece = g_charToPieceUSI.value(token);
                setHand(piece, (digits == 0 ? 1 : digits));
                
                digits = 0;
            }
            else
                goto INCORRECT;
        }
        
        // 次の手が何手目か
        ss >> gamePly_;
        
        // 残り時間, hash key, (もし実装するなら)駒番号などをここで設定
        st_->boardKey = computeBoardKey();
        st_->handKey = computeHandKey();
        st_->hand = hand(turn());
        
        setEvalList();
        findCheckers();
        st_->material = computeMaterial();
        thisThread_ = th;
        
        return;
    INCORRECT:
        std::cout << "incorrect SFEN string : " << sfen << std::endl;
    }
    
    bool Position::set(const HuffmanCodedPos& hcp, Thread* th) {
        Searcher* s = std::move(searcher_);
        clear();
        setSearcher(s);
        
        HuffmanCodedPos tmp = hcp; // ローカルにコピー
        BitStream bs(tmp.data);
        
        // 手番
        turn_ = static_cast<Color>(bs.getBit());
        
        // 玉の位置
        Square sq0 = (Square)bs.getBits(7);
        Square sq1 = (Square)bs.getBits(7);
        setPiece(BKing, static_cast<Square>(sq0));
        setPiece(WKing, static_cast<Square>(sq1));
        
        // 盤上の駒
        for (Square sq = SQ11; sq < SquareNum; ++sq) {
            if (pieceToPieceType(piece(sq)) == King) // piece(sq) は BKing, WKing, Empty のどれか。
                continue;
            HuffmanCode hc = {0, 0};
            while (hc.numOfBits <= 8) {
                hc.code |= bs.getBit() << hc.numOfBits++;
                if (HuffmanCodedPos::boardCodeToPieceHash.value(hc.key) != PieceNone) {
                    const Piece pc = HuffmanCodedPos::boardCodeToPieceHash.value(hc.key);
                    if (pc != Empty)
                        setPiece(HuffmanCodedPos::boardCodeToPieceHash.value(hc.key), sq);
                    break;
                }
            }
            if (HuffmanCodedPos::boardCodeToPieceHash.value(hc.key) == PieceNone)
                goto INCORRECT_HUFFMAN_CODE;
        }
        while (bs.data() != std::end(tmp.data)) {
            HuffmanCode hc = {0, 0};
            while (hc.numOfBits <= 8) {
                hc.code |= bs.getBit() << hc.numOfBits++;
                const Piece pc = HuffmanCodedPos::handCodeToPieceHash.value(hc.key);
                if (pc != PieceNone) {
                    hand_[pieceToColor(pc)].plusOne(pieceTypeToHandPiece(pieceToPieceType(pc)));
                    break;
                }
            }
            if (HuffmanCodedPos::handCodeToPieceHash.value(hc.key) == PieceNone)
                goto INCORRECT_HUFFMAN_CODE;
        }
        
        kingSquare_[Black] = bbOf(King, Black).constFirstOneFromSQ11();
        kingSquare_[White] = bbOf(King, White).constFirstOneFromSQ11();
        goldsBB_ = bbOf(Gold, ProPawn, ProLance, ProKnight, ProSilver);
        
        gamePly_ = 1; // ply の情報は持っていないので 1 にしておく。
        
        st_->boardKey = computeBoardKey();
        st_->handKey = computeHandKey();
        st_->hand = hand(turn());
        
        setEvalList();
        findCheckers();
        st_->material = computeMaterial();
        thisThread_ = th;
        
        return true;
    INCORRECT_HUFFMAN_CODE:
        std::cout << "incorrect Huffman code." << std::endl;
        return false;
    }
    
    // ランダムな局面を作成する。
    void Position::set(std::mt19937& mt, Thread* th) {
        Searcher* s = std::move(searcher_);
        clear();
        setSearcher(s);
        
        // 手番の設定。
        std::uniform_int_distribution<int> colorDist(0, (int)ColorNum - 1);
        turn_ = (Color)colorDist(mt);
        
        // 先後両方の持ち駒の数を設定する。持ち駒が多くなるほど、確率が低くなるので、取り敢えずこれで良しとする。todo: 確率分布を指定出来るように。
        auto setHandPieces = [&](const HandPiece hp, const int32_t maxNum) {
            std::uniform_int_distribution<int> handNumDist(0, maxNum);
            while (true) {
                const int32_t nums[ColorNum] = {handNumDist(mt), handNumDist(mt)};
                if (nums[Black] + nums[White] <= maxNum) {
                    setHand(hp, Black, nums[Black]);
                    setHand(hp, White, nums[White]);
                }
                break;
            }
        };
        setHandPieces(HPawn  , 18);
        setHandPieces(HLance ,  4);
        setHandPieces(HKnight,  4);
        setHandPieces(HSilver,  4);
        setHandPieces(HGold  ,  4);
        setHandPieces(HBishop,  2);
        setHandPieces(HRook  ,  2);
        
        // 玉の位置の設定。
        std::uniform_int_distribution<int> squareDist(0, (int)SquareNum - 1);
        while (true) {
            const Square ksqs[ColorNum] = {(Square)squareDist(mt), (Square)squareDist(mt)};
            // 玉同士が同じ位置、もしくは相手の玉の利きにいる事は絶対にない。その他なら何でも良い。
            if (ksqs[Black] == ksqs[White])
                continue;
            if (kingAttack(ksqs[Black]) & setMaskBB(ksqs[White]))
                continue;
            // 先手の玉の利きに後手玉がいなければ、後手玉の利きにも先手玉はいない。
            //if (kingAttack(ksqs[White]) & setMaskBB(ksqs[Black]))
            //    continue;
            setPiece(BKing, ksqs[Black]);
            setPiece(WKing, ksqs[White]);
            kingSquare_[Black] = ksqs[Black];
            kingSquare_[White] = ksqs[White];
            break;
        }
        
        // なる為の閾値。これ以上だと成っているとみなす。[0,99]
        // todo: 1段目が選ばれて、成っていなければ、歩香桂はやり直し。分布が偏るが、どういう分布が良いかも分からないのである程度は適当に。
        static const int32_t promoteThresh[ColorNum][RankNum] = {{30, 30, 30, 40, 60, 90, 99, 99, 99},
            {99, 99, 99, 90, 60, 40, 30, 30, 30}};

        int32_t checkersNum = 0;
        Square checkSquare = SquareNum; // 1つ目の王手している駒の位置。(1つしか保持する必要が無い。)
        // 飛び利きの無い駒の配置。
        auto shortPiecesSet = [&](const PieceType pt, const HandPiece hp, const int32_t maxNum) {
            for (int32_t i = 0; i < maxNum - (int32_t)(hand(Black).numOf(hp) + hand(White).numOf(hp)); ++i) {
                while (true) {
                    const Square sq = (Square)squareDist(mt);
                    // その場所に既に駒があるか。
                    if (occupiedBB().isSet(sq))
                        continue;
                    const File file = makeFile(sq);
                    const Rank rank = makeRank(sq);
                    const Color t = (Color)colorDist(mt);
                    // 出来るだけ前にいるほど成っている確率を高めに。
                    std::uniform_int_distribution<int> promoteDist(0, 99);
                    const Piece promoteFlag = (pt != Gold && promoteThresh[t][rank] <= promoteDist(mt) ? Promoted : UnPromoted);
                    const Piece pc = colorAndPieceTypeToPiece(t, pt) + promoteFlag;
                    if (promoteFlag == UnPromoted) {
                        if (pt == Pawn) {
                            // 二歩のチェック
                            if (fileMask(file) & bbOf(Pawn, t))
                                continue;
                            // 行き所が無いかチェック
                            if (t == Black)
                                if (isInFrontOf<Black, Rank2, Rank8>(rank))
                                    continue;
                            if (t == White)
                                if (isInFrontOf<White, Rank2, Rank8>(rank))
                                    continue;
                        }
                        else if (pt == Knight) {
                            // 行き所が無いかチェック
                            if (t == Black)
                                if (isInFrontOf<Black, Rank3, Rank7>(rank))
                                    continue;
                            if (t == White)
                                if (isInFrontOf<White, Rank3, Rank7>(rank))
                                    continue;
                        }
                    }
                    if (t == turn()) {
                        // 手番側が王手していてはいけない。
                        if (attacksFrom(pieceToPieceType(pc), t, sq).isSet(kingSquare(oppositeColor(turn()))))
                            continue;
                    }
                    else {
                        if (attacksFrom(pieceToPieceType(pc), t, sq).isSet(kingSquare(turn()))) {
                            // 王手の位置。
                            // 飛び利きでない駒のみでは同時に1つの駒しか王手出来ない。
                            if (checkersNum != 0)
                                continue;
                            ++checkersNum;
                            checkSquare = sq;
                        }
                    }
                    setPiece(pc, sq);
                    break;
                }
            }
        };
        shortPiecesSet(Pawn  , HPawn  , 18);
        shortPiecesSet(Knight, HKnight,  4);
        shortPiecesSet(Silver, HSilver,  4);
        shortPiecesSet(Gold  , HGold  ,  4);
        
        // 飛び利きの駒を配置。
        auto longPiecesSet = [&](const PieceType pt, const HandPiece hp, const int32_t maxNum) {
            for (int32_t i = 0; i < maxNum - (int32_t)(hand(Black).numOf(hp) + hand(White).numOf(hp)); ++i) {
                while (true) {
                    const Square sq = (Square)squareDist(mt);
                    // その場所に既に駒があるか。
                    if (occupiedBB().isSet(sq))
                        continue;
                    const File file = makeFile(sq);
                    const Rank rank = makeRank(sq);
                    const Color t = (Color)colorDist(mt);
                    // 出来るだけ前にいるほど成っている確率を高めに。
                    std::uniform_int_distribution<int> promoteDist(0, 99);
                    const Piece promoteFlag = [&] {
                        if (pt == Lance)
                            return (promoteThresh[t][rank] <= promoteDist(mt) ? Promoted : UnPromoted);
                        else // 飛角に関しては、成りと不成は同確率とする。
                            return (50 <= promoteDist(mt) ? Promoted : UnPromoted);
                    }();
                    const Piece pc = colorAndPieceTypeToPiece(t, pt) + promoteFlag;
                    if (promoteFlag == UnPromoted) {
                        if (pt == Lance) {
                            // 行き所が無いかチェック
                            if (t == Black)
                                if (isInFrontOf<Black, Rank2, Rank8>(rank))
                                    continue;
                            if (t == White)
                                if (isInFrontOf<White, Rank2, Rank8>(rank))
                                    continue;
                        }
                    }
                    // 手番側が王手していないか。
                    if (t == turn()) {
                        if (attacksFrom(pieceToPieceType(pc), t, sq).isSet(kingSquare(oppositeColor(turn()))))
                            continue;
                    }
                    else {
                        if (attacksFrom(pieceToPieceType(pc), t, sq).isSet(kingSquare(turn()))) {
                            // 王手の位置。
                            // 飛び利きを含めて同時に王手する駒は2つまで。既に2つあるならそれ以上王手出来ない。
                            if (checkersNum >= 2)
                                continue;
                            if (checkersNum == 1) {
                                // 両王手。両王手は少なくとも1つは遠隔王手である必要がある。
                                // この駒は近接王手か。
                                if (kingAttack(kingSquare(turn())) & setMaskBB(sq)) {
                                    // もう1つの王手している駒も近接王手か。
                                    if (kingAttack(kingSquare(turn())) & setMaskBB(checkSquare))
                                        continue; // 両方が近接王ではあり得ない。
                                    // もう1つの王手している駒が遠隔王手。
                                    // この駒の位置を戻すともう1つの王手を隠せる必要がある。
                                    if (!(attacksFrom(pieceToPieceType(piece(sq)), oppositeColor(t), sq) & betweenBB(kingSquare(turn()), checkSquare))) {
                                        // 王手を隠せなかったので、この駒が成り駒であれば、成る前の動きでチェック。
                                        if (!(piece(sq) & Promoted))
                                            continue; // 成り駒ではなかった。
                                        const Bitboard hiddenBB = attacksFrom(pieceToPieceType(piece(sq)) - Promoted, oppositeColor(t), sq) & betweenBB(kingSquare(turn()), checkSquare);
                                        if (!hiddenBB)
                                            continue;
                                        // 成る前の利きならもう一方の王手を隠せた。
                                        // 後は、この駒が成っていない状態から、sq に移動して成れたか。
                                        if (!canPromote(t, makeRank(sq)) && !(hiddenBB & (t == Black ? inFrontMask<Black, Rank4>() : inFrontMask<White, Rank6>())))
                                            continue; // from, to 共に敵陣ではないので、成る事が出来ない。
                                    }
                                }
                                else {
                                    // この駒は遠隔王手。
                                    if (!(attacksFrom(pieceToPieceType(piece(checkSquare)), oppositeColor(t), checkSquare) & betweenBB(kingSquare(turn()), sq))) {
                                        // この駒の王手を隠せなかった。
                                        if (!(piece(checkSquare) & Promoted))
                                            continue; // もう一方の王手している駒が成り駒ではなかった。
                                        const Bitboard hiddenBB = attacksFrom(pieceToPieceType(piece(checkSquare)) - Promoted, oppositeColor(t), checkSquare) & betweenBB(kingSquare(turn()), sq);
                                        if (!hiddenBB)
                                            continue;
                                        // 成る前の利きならこの駒の王手を隠せた。
                                        // 後は、もう一方の王手している駒が成っていない状態から、checkSquare に移動して成れたか。
                                        if (!canPromote(t, makeRank(checkSquare)) && !(hiddenBB & (t == Black ? inFrontMask<Black, Rank4>() : inFrontMask<White, Rank6>())))
                                            continue; // from, to 共に敵陣ではないので、成る事が出来ない。
                                    }
                                }
                                // 両王手でこの2つの駒が王手しているのはあり得る。
                                // ただし、これ以上厳密には判定しないが、他の駒を盤上に置く事で、この両王手が成り立たなくなる可能性はある。
                            }
                            else // 最初の王手の駒なので、位置を保持する。
                                checkSquare = sq;
                            ++checkersNum;
                        }
                    }
                    setPiece(pc, sq);
                    break;
                }
            }
        };
        longPiecesSet(Lance , HLance , 4);
        longPiecesSet(Bishop, HBishop, 2);
        longPiecesSet(Rook  , HRook  , 2);
        
        goldsBB_ = bbOf(Gold, ProPawn, ProLance, ProKnight, ProSilver);
        
        gamePly_ = 1; // ply の情報は持っていないので 1 にしておく。
        
        st_->boardKey = computeBoardKey();
        st_->handKey = computeHandKey();
        st_->hand = hand(turn());
        
        setEvalList();
        findCheckers();
        st_->material = computeMaterial();
        thisThread_ = th;
    }
    
    bool Position::moveGivesCheck(const Move move) const {
        return moveGivesCheck(move, CheckInfo(*this));
    }
    
    // move が王手なら true
    bool Position::moveGivesCheck(const Move move, const CheckInfo& ci) const {
        // assert(isOK());
        if(!isOK()){
            printf("error, assert(isOK())\n");
        }
        // assert(ci.dcBB == discoveredCheckBB());
        if(!(ci.dcBB == discoveredCheckBB())){
            printf("error, assert(ci.dcBB == discoveredCheckBB())\n");
        }
        
        const Square to = move.to();
        if (move.isDrop()) {
            const PieceType ptTo = move.pieceTypeDropped();
            // Direct Check ?
            if (ci.checkBB[ptTo].isSet(to))
                return true;
        }
        else {
            const Square from = move.from();
            const PieceType ptFrom = move.pieceTypeFrom();
            const PieceType ptTo = move.pieceTypeTo(ptFrom);
            
            // assert(ptFrom == pieceToPieceType(piece(from)));
            if(!(ptFrom == pieceToPieceType(piece(from)))){
                printf("error, assert(ptFrom == pieceToPieceType(piece(from)))\n");
            }
            
            // Direct Check ?
            if (ci.checkBB[ptTo].isSet(to))
                return true;
            
            // Discovery Check ?
            if (isDiscoveredCheck(from, to, kingSquare(oppositeColor(turn())), ci.dcBB))
                return true;
        }
        
        return false;
    }
    
    Piece Position::movedPiece(const Move m) const {
        return colorAndPieceTypeToPiece(turn(), m.pieceTypeFromOrDropped());
    }
    
    void Position::clear() {
        memset(this, 0, sizeof(Position));
        st_ = &startState_;
    }
    
    // 先手、後手に関わらず、sq へ移動可能な Bitboard を返す。
    Bitboard Position::attackersTo(const Square sq, const Bitboard& occupied) const {
        const Bitboard golds = goldsBB();
        return (((attacksFrom<Pawn  >(Black, sq          ) & bbOf(Pawn  ))
                 | (attacksFrom<Lance >(Black, sq, occupied) & bbOf(Lance ))
                 | (attacksFrom<Knight>(Black, sq          ) & bbOf(Knight))
                 | (attacksFrom<Silver>(Black, sq          ) & bbOf(Silver))
                 | (attacksFrom<Gold  >(Black, sq          ) & golds       ))
                & bbOf(White))
        | (((attacksFrom<Pawn  >(White, sq          ) & bbOf(Pawn  ))
            | (attacksFrom<Lance >(White, sq, occupied) & bbOf(Lance ))
            | (attacksFrom<Knight>(White, sq          ) & bbOf(Knight))
            | (attacksFrom<Silver>(White, sq          ) & bbOf(Silver))
            | (attacksFrom<Gold  >(White, sq          ) & golds))
           & bbOf(Black))
        | (attacksFrom<Bishop>(sq, occupied) & bbOf(Bishop, Horse        ))
        | (attacksFrom<Rook  >(sq, occupied) & bbOf(Rook  , Dragon       ))
        | (attacksFrom<King  >(sq          ) & bbOf(King  , Horse, Dragon));
    }
    
    // occupied を Position::occupiedBB() 以外のものを使用する場合に使用する。
    Bitboard Position::attackersTo(const Color c, const Square sq, const Bitboard& occupied) const {
        const Color opposite = oppositeColor(c);
        return ((attacksFrom<Pawn  >(opposite, sq          ) & bbOf(Pawn  ))
                | (attacksFrom<Lance >(opposite, sq, occupied) & bbOf(Lance ))
                | (attacksFrom<Knight>(opposite, sq          ) & bbOf(Knight))
                | (attacksFrom<Silver>(opposite, sq          ) & bbOf(Silver, King, Dragon))
                | (attacksFrom<Gold  >(opposite, sq          ) & (bbOf(King  , Horse) | goldsBB()))
                | (attacksFrom<Bishop>(          sq, occupied) & bbOf(Bishop, Horse        ))
                | (attacksFrom<Rook  >(          sq, occupied) & bbOf(Rook  , Dragon       )))
        & bbOf(c);
    }
    
    // 玉以外で sq へ移動可能な c 側の駒の Bitboard を返す。
    Bitboard Position::attackersToExceptKing(const Color c, const Square sq) const {
        const Color opposite = oppositeColor(c);
        return ((attacksFrom<Pawn  >(opposite, sq) & bbOf(Pawn  ))
                | (attacksFrom<Lance >(opposite, sq) & bbOf(Lance ))
                | (attacksFrom<Knight>(opposite, sq) & bbOf(Knight))
                | (attacksFrom<Silver>(opposite, sq) & bbOf(Silver, Dragon))
                | (attacksFrom<Gold  >(opposite, sq) & (goldsBB() | bbOf(Horse)))
                | (attacksFrom<Bishop>(          sq) & bbOf(Bishop, Horse ))
                | (attacksFrom<Rook  >(          sq) & bbOf(Rook  , Dragon)))
        & bbOf(c);
    }
    
    Score Position::computeMaterial() const {
        Score s = ScoreZero;
        for (PieceType pt = Pawn; pt < PieceTypeNum; ++pt) {
            const int32_t num = bbOf(pt, Black).popCount() - bbOf(pt, White).popCount();
            s += num * pieceScore(pt);
        }
        for (PieceType pt = Pawn; pt < King; ++pt) {
            const int32_t num = hand(Black).numOf(pieceTypeToHandPiece(pt)) - hand(White).numOf(pieceTypeToHandPiece(pt));
            s += num * pieceScore(pt);
        }
        return s;
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////// Convert ///////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////

    int32_t Position::getByteSize()
    {
        int32_t ret = 0;
        {
            // Bitboard size: 2 u64
            int32_t bitboardSize = 2*sizeof(u64);
            // Bitboard byTypeBB_[PieceTypeNum];
            ret+=PieceTypeNum*bitboardSize;
            // Bitboard byColorBB_[ColorNum];
            ret+=ColorNum*bitboardSize;
            // Bitboard goldsBB_;
            ret+=bitboardSize;
            
            // 各マスの状態
            // Piece piece_[SquareNum];
            ret+=(int32_t)SquareNum*sizeof(int32_t);
            // Square kingSquare_[ColorNum];
            ret+=(int32_t)ColorNum*sizeof(int32_t);
            
            // 手駒
            // Hand hand_[ColorNum];
            {
                int32_t sizeOfHand = sizeof(u32);
                ret+=ColorNum*sizeOfHand;
            }
            // Color turn_;
            ret+=sizeof(int32_t);
            
            // EvalList evalList_;
            {
                // EvalIndex list0[ListSize];
                ret+=sizeof(int32_t)*EvalList::ListSize;
                // EvalIndex list1[ListSize];
                ret+=sizeof(int32_t)*EvalList::ListSize;
                // Square listToSquareHand[ListSize];
                ret+=sizeof(int32_t)*EvalList::ListSize;
                // int32_t squareHandToList[SquareHandNum];
                ret+=sizeof(int32_t)*(int)SquareHandNum;
            }
            
            {
                // find stateInfo size
                int32_t stateInfoSize = StateInfo::getStateInfoByteSize();
                // StateInfo startState_;
                {
                    // write stateInfo number
                    ret+=sizeof(int32_t);
                    // find stateInfo count
                    int32_t stateInfoCount = 0;
                    {
                        StateInfo* temp = &startState_;
                        while (temp!=NULL) {
                            stateInfoCount++;
                            temp = temp->previous;
                        }
                    }
                    // add size
                    ret+=stateInfoSize*stateInfoCount;
                    // printf("stateInfoCount: startState_: %d\n", stateInfoCount);
                }
                // StateInfo* st_;
                {
                    // write stateInfo number
                    ret+=sizeof(int32_t);
                    // find stateInfo count
                    int32_t stateInfoCount = 0;
                    {
                        StateInfo* temp = st_;
                        while (temp!=NULL) {
                            stateInfoCount++;
                            temp = temp->previous;
                        }
                    }
                    // add size
                    ret+=stateInfoSize*stateInfoCount;
                    // printf("stateInfoCount: st_: %d\n", stateInfoCount);
                }
            }
            // 時間管理に使用する。
            // Ply gamePly_;
            ret+=sizeof(int32_t);
            // s64 nodes_;
            ret+=sizeof(s64);
        }
        return ret;
    }

    int32_t Position::convertToByteArray(Position* position, uint8_t* &byteArray)
    {
        int32_t length = position->getByteSize();
        u8* ret = (uint8_t*)calloc(length, sizeof(uint8_t));
        {
            int32_t pointerIndex = 0;
            // Bitboard byTypeBB_[PieceTypeNum];
            {
                int32_t size = 2*sizeof(u64);// bitboardSize
                for(int32_t i=0; i<PieceTypeNum; i++){
                    if(pointerIndex+size<=length){
                        u64* value = position->byTypeBB_[i].getValue();
                        memcpy(ret + pointerIndex, value, size);
                        pointerIndex+=size;
                    }else{
                        printf("length error: byTypeBB: %d, %d\n", pointerIndex, length);
                    }
                }
            }
            // Bitboard byColorBB_[ColorNum];
            {
                int32_t size = 2*sizeof(u64);// bitboardSize
                for(int32_t i=0; i<ColorNum; i++){
                    if(pointerIndex+size<=length){
                        u64* value = position->byColorBB_[i].getValue();
                        memcpy(ret + pointerIndex, value, size);
                        pointerIndex+=size;
                    }else{
                        printf("length error: byColorBB: %d, %d\n", pointerIndex, length);
                    }
                }
            }
            // Bitboard goldsBB_;
            {
                int32_t size = 2*sizeof(u64);// bitboardSize
                if(pointerIndex+size<=length){
                    u64* value = position->goldsBB_.getValue();
                    memcpy(ret + pointerIndex, value, size);
                    pointerIndex+=size;
                }else{
                    printf("length error: goldsBB: %d, %d\n", pointerIndex, length);
                }
            }
            
            // 各マスの状態
            // Piece piece_[SquareNum];
            {
                int32_t size = sizeof(int32_t);
                for(int32_t i=0; i<SquareNum; i++){
                    if(pointerIndex+size<=length){
                        int32_t value = position->piece_[i];
                        memcpy(ret + pointerIndex, &value, size);
                        pointerIndex+=size;
                    }else{
                        printf("length error: piece: %d, %d\n", pointerIndex, length);
                    }
                }
            }
            // Square kingSquare_[ColorNum];
            {
                int32_t size = sizeof(int32_t);
                for(int32_t i=0; i<ColorNum; i++){
                    if(pointerIndex+size<=length){
                        int32_t value = position->kingSquare_[i];
                        memcpy(ret + pointerIndex, &value, size);
                        pointerIndex+=size;
                    }else{
                        printf("length error: kingSquare: %d, %d\n", pointerIndex, length);
                    }
                }
            }
            
            // 手駒
            // Hand hand_[ColorNum];
            {
                int32_t size = sizeof(u32);
                for(int32_t i=0; i<ColorNum; i++){
                    if(pointerIndex+size<=length){
                        u32 value = position->hand_[i].value_;
                        memcpy(ret + pointerIndex, &value, size);
                        pointerIndex+=size;
                    }else{
                        printf("length error: hand: %d, %d\n", pointerIndex, length);
                    }
                }
            }
            // Color turn_;
            {
                int32_t size = sizeof(int32_t);
                if(pointerIndex+size<=length){
                    int32_t value = position->turn_;
                    memcpy(ret + pointerIndex, &value, size);
                    pointerIndex+=size;
                }else{
                    printf("length error: turn: %d, %d\n", pointerIndex, length);
                }
            }
            
            // EvalList evalList_;
            {
                EvalList evalList = position->evalList_;
                // EvalIndex list0[ListSize];
                {
                    int32_t size = sizeof(int32_t);
                    for(int32_t i=0; i<EvalList::ListSize; i++){
                        if(pointerIndex+size<=length){
                            int32_t value = evalList.list0[i];
                            memcpy(ret + pointerIndex, &value, size);
                            pointerIndex+=size;
                        }else{
                            printf("length error: EvalIndex list0: %d, %d\n", pointerIndex, length);
                        }
                    }
                }
                // EvalIndex list1[ListSize];
                {
                    int32_t size = sizeof(int32_t);
                    for(int32_t i=0; i<EvalList::ListSize; i++){
                        if(pointerIndex+size<=length){
                            int32_t value = evalList.list1[i];
                            memcpy(ret + pointerIndex, &value, size);
                            pointerIndex+=size;
                        }else{
                            printf("length error: EvalIndex list1: %d, %d\n", pointerIndex, length);
                        }
                    }
                }
                // Square listToSquareHand[ListSize];
                {
                    int32_t size = sizeof(int32_t);
                    for(int32_t i=0; i<EvalList::ListSize; i++){
                        if(pointerIndex+size<=length){
                            int32_t value = evalList.listToSquareHand[i];
                            memcpy(ret + pointerIndex, &value, size);
                            pointerIndex+=size;
                        }else{
                            printf("length error: Square listToSquareHand: %d, %d\n", pointerIndex, length);
                        }
                    }
                }
                // int squareHandToList[SquareHandNum];
                {
                    int32_t size = sizeof(int32_t);
                    for(int32_t i=0; i<SquareHandNum; i++){
                        if(pointerIndex+size<=length){
                            int32_t value = evalList.squareHandToList[i];
                            memcpy(ret + pointerIndex, &value, size);
                            pointerIndex+=size;
                        }else{
                            printf("length error: int32_t squareHandToList: %d, %d\n", pointerIndex, length);
                        }
                    }
                }
            }
            
            {
                // StateInfo startState_;
                {
                    // get stateInfo list
                    std::vector<StateInfo*> sts;
                    {
                        // lay theo thu tu tu turn cao xuong turn thap
                        StateInfo* temp = &position->startState_;
                        while (temp!=NULL) {
                            sts.push_back(temp);
                            temp = temp->previous;
                        }
                    }
                    // write stateInfo number
                    {
                        int32_t size = sizeof(int32_t);
                        if(pointerIndex+size<=length){
                            int32_t value = (int32_t)sts.size();
                            memcpy(ret + pointerIndex, &value, size);
                            pointerIndex+=size;
                        }else{
                            printf("length error: stateInfo number: %d, %d\n", pointerIndex, length);
                        }
                    }
                    // write all stateInfo: theo thu tu tu turn thap den turn cao
                    {
                        for(int32_t i=(int32_t)(sts.size()-1); i>=0; i--){
                            StateInfo* st = sts[i];
                            // convert stateInfo into byteArray
                            u8* stByteArray;
                            int32_t size = StateInfo::convertToByteArray (st, stByteArray);
                            if(pointerIndex+size<=length){
                                memcpy(ret + pointerIndex, stByteArray, size);
                                pointerIndex+=size;
                            }else{
                                printf("length error: stateInfo: %d, %d\n", pointerIndex, length);
                            }
                            free(stByteArray);
                        }
                    }
                }
                // StateInfo* st_;
                {
                    // get stateInfo list
                    std::vector<StateInfo*> sts;
                    {
                        // lay theo thu tu tu turn cao xuong turn thap
                        StateInfo* temp = position->st_;
                        while (temp!=NULL) {
                            sts.push_back(temp);
                            temp = temp->previous;
                        }
                    }
                    // write stateInfo number
                    {
                        int32_t size = sizeof(int32_t);
                        if(pointerIndex+size<=length){
                            int32_t value = (int)sts.size();
                            memcpy(ret + pointerIndex, &value, size);
                            pointerIndex+=size;
                        }else{
                            printf("length error: stateInfo number: %d, %d\n", pointerIndex, length);
                        }
                    }
                    // write all stateInfo into byteArray: theo thu tu tu turn thap den turn cao
                    {
                        for(int32_t i=(int32_t)(sts.size()-1); i>=0; i--){
                            StateInfo* st = sts[i];
                            // convert stateInfo into byteArray
                            u8* stByteArray;
                            int32_t size = StateInfo::convertToByteArray (st, stByteArray);
                            if(pointerIndex+size<=length){
                                memcpy(ret + pointerIndex, stByteArray, size);
                                pointerIndex+=size;
                            }else{
                                printf("length error: stateInfo: %d, %d\n", pointerIndex, length);
                            }
                            free(stByteArray);
                        }
                    }
                }
            }
            // 時間管理に使用する。
            // Ply gamePly_;
            {
                int32_t size = sizeof(int32_t);
                if(pointerIndex+size<=length){
                    int32_t value = position->gamePly_;
                    memcpy(ret + pointerIndex, &value, size);
                    pointerIndex+=size;
                }else{
                    printf("length error: gamePly: %d, %d\n", pointerIndex, length);
                }
            }
            // s64 nodes_;
            {
                int32_t size = sizeof(s64);
                if(pointerIndex+size<=length){
                    s64 value = position->nodes_;
                    memcpy(ret + pointerIndex, &value, size);
                    pointerIndex+=size;
                }else{
                    printf("length error: nodes: %d, %d\n", pointerIndex, length);
                }
            }
        }
        byteArray = ret;
        return length;
    }

    int32_t Position::parse(Position* position, u8* positionBytes, int32_t length, int32_t start, bool canCorrect)
    {
        int32_t pointerIndex = start;
        int32_t index = 0;
        bool isParseCorrect = true;
        while (pointerIndex < length) {
            bool alreadyPassAll = false;
            switch (index) {
                case 0:
                {
                    // Bitboard byTypeBB_[PieceTypeNum];
                    int32_t size = 2*sizeof(u64);// bitboardSize
                    for(int32_t i=0; i<PieceTypeNum; i++){
                        if(pointerIndex+size<=length){
                            memcpy(position->byTypeBB_[i].getValue(), positionBytes + pointerIndex, size);
                            pointerIndex+=size;
                        }else{
                            printf("length error: byTybeBB_: %d, %d\n", pointerIndex, length);
                            isParseCorrect = false;
                        }
                    }
                }
                    break;
                case 1:
                {
                    // Bitboard byColorBB_[ColorNum];
                    int32_t size = 2*sizeof(u64);// bitboardSize
                    for(int32_t i=0; i<ColorNum; i++){
                        if(pointerIndex+size<=length){
                            memcpy(position->byColorBB_[i].getValue(), positionBytes + pointerIndex, size);
                            pointerIndex+=size;
                        }else{
                            printf("length error: byColorBB_: %d, %d\n", pointerIndex, length);
                            isParseCorrect = false;
                        }
                    }
                }
                    break;
                case 2:
                    // Bitboard goldsBB_;
                {
                    int32_t size = 2*sizeof(u64);// bitboardSize
                    if(pointerIndex+size<=length){
                        memcpy(position->goldsBB_.getValue(), positionBytes + pointerIndex, size);
                        pointerIndex+=size;
                    }else{
                        printf("length error: goldsBB_: %d, %d\n", pointerIndex, length);
                        isParseCorrect = false;
                    }
                }
                    break;
                case 3:
                    // Piece piece_[SquareNum];
                {
                    int32_t size = sizeof(int32_t);
                    for(int32_t i=0; i<SquareNum; i++){
                        if(pointerIndex+size<=length){
                            memcpy(&position->piece_[i], positionBytes + pointerIndex, size);
                            pointerIndex+=size;
                        }else{
                            printf("length error: piece_: %d, %d\n", pointerIndex, length);
                            isParseCorrect = false;
                        }
                    }
                }
                    break;
                case 4:
                    // Square kingSquare_[ColorNum];
                {
                    int32_t size = sizeof(int32_t);
                    for(int32_t i=0; i<ColorNum; i++){
                        if(pointerIndex+size<=length){
                            memcpy(&position->kingSquare_[i], positionBytes + pointerIndex, size);
                            pointerIndex+=size;
                        }else{
                            printf("length error: kingSquare_: %d, %d\n", pointerIndex, length);
                            isParseCorrect = false;
                        }
                    }
                }
                    break;
                case 5:
                    // Hand hand_[ColorNum];
                {
                    int32_t size = sizeof(u32);
                    for(int32_t i=0; i<ColorNum; i++){
                        if(pointerIndex+size<=length){
                            memcpy(&position->hand_[i].value_, positionBytes + pointerIndex, size);
                            pointerIndex+=size;
                        }else{
                            printf("length error: hand_: %d, %d\n", pointerIndex, length);
                            isParseCorrect = false;
                        }
                    }
                }
                    break;
                case 6:
                    // Color turn_;
                {
                    int32_t size = sizeof(int32_t);
                    if(pointerIndex+size<=length){
                        memcpy(&position->turn_, positionBytes + pointerIndex, size);
                        pointerIndex+=size;
                    }else{
                        printf("length error: turn_: %d, %d\n", pointerIndex, length);
                        isParseCorrect = false;
                    }
                }
                    break;
                case 7:
                    // EvalList evalList_;
                {
                    // EvalIndex list0[ListSize];
                    {
                        int32_t size = sizeof(int32_t);
                        for(int32_t i=0; i<EvalList::ListSize; i++){
                            if(pointerIndex+size<=length){
                                memcpy(&position->evalList_.list0[i], positionBytes + pointerIndex, size);
                                pointerIndex+=size;
                            }else{
                                printf("length error: evalList: %d, %d\n", pointerIndex, length);
                                isParseCorrect = false;
                            }
                        }
                    }
                    // EvalIndex list1[ListSize];
                    {
                        int32_t size = sizeof(int32_t);
                        for(int32_t i=0; i<EvalList::ListSize; i++){
                            if(pointerIndex+size<=length){
                                memcpy(&position->evalList_.list1[i], positionBytes + pointerIndex, size);
                                pointerIndex+=size;
                            }else{
                                printf("length error: evalList_: %d, %d\n", pointerIndex, length);
                                isParseCorrect = false;
                            }
                        }
                    }
                    // Square listToSquareHand[ListSize];
                    {
                        int32_t size = sizeof(int32_t);
                        for(int32_t i=0; i<EvalList::ListSize; i++){
                            if(pointerIndex+size<=length){
                                memcpy(&position->evalList_.listToSquareHand[i], positionBytes + pointerIndex, size);
                                pointerIndex+=size;
                            }else{
                                printf("length error: evalList_: listToSquareHand: %d, %d\n", pointerIndex, length);
                                isParseCorrect = false;
                            }
                        }
                    }
                    // int32_t squareHandToList[SquareHandNum];
                    {
                        int32_t size = sizeof(int32_t);
                        for(int32_t i=0; i<SquareHandNum; i++){
                            if(pointerIndex+size<=length){
                                memcpy(&position->evalList_.squareHandToList[i], positionBytes + pointerIndex, size);
                                pointerIndex+=size;
                            }else{
                                printf("length error: evalList_: squareHandToList: %d, %d\n", pointerIndex, length);
                                isParseCorrect = false;
                            }
                        }
                    }
                }
                    break;
                case 8:
                {
                    // StateInfo startState_;
                    // remove old stateInfo
                    {
                        // get list
                        std::vector<StateInfo*> sts;
                        {
                            StateInfo* temp = position->startState_.previous;
                            while (temp!=NULL) {
                                sts.push_back(temp);
                                temp = temp->previous;
                            }
                        }
                        // free
                        /*for(int32_t i=0; i<sts.size(); i++){
                            StateInfo* st = sts[i];
                            free(st);
                        }*/
                        position->startState_.previous = NULL;
                    }
                    // find stateInfoNumber
                    int32_t stateInfoNumber = 0;
                    {
                        int32_t size = sizeof(int32_t);
                        if(pointerIndex+size<=length){
                            memcpy(&stateInfoNumber, positionBytes + pointerIndex, size);
                            pointerIndex+=size;
                        }else{
                            printf("length error: startState_: %d, %d\n", pointerIndex, length);
                            isParseCorrect = false;
                        }
                    }
                    // parse
                    {
                        std::vector<StateInfo*> sts;
                        // parse
                        {
                            for (int32_t i = 0; i < stateInfoNumber; i++) {
                                // get stateInfo
                                StateInfo* st;
                                {
                                    if(i==stateInfoNumber-1){
                                        st = &position->startState_;
                                    }else{
                                        st = (StateInfo*)calloc(1, sizeof(StateInfo));
                                        position->pushStateInfo(st);
                                    }
                                }
                                // parse
                                int32_t stateInfoByteLength = StateInfo::parse (st, positionBytes, length, pointerIndex);
                                if (stateInfoByteLength > 0) {
                                    // add to chess
                                    {
                                        if(i>0){
                                            sts.push_back(st);
                                        }
                                    }
                                    pointerIndex+= stateInfoByteLength;
                                } else {
                                    printf("cannot parse\n");
                                    if(i>0){
                                        // free(st);
                                    }else{
                                        printf("error, why cannot parse startState_\n");
                                    }
                                    break;
                                }
                            }
                        }
                        // Dat stateInfo vao vi tri cua position
                        {
                            // vector theo thu tu tu turn thap den turn cao
                            for(int32_t i=0; i<sts.size(); i++){
                                // lay turn cuoi dat vao position
                                if(i==sts.size()-1){
                                    position->startState_.previous = sts[0];
                                }
                                if(i!=0){
                                    sts[i]->previous = sts[i-1];
                                }else{
                                    sts[i]->previous = NULL;
                                }
                            }
                        }
                    }
                }
                    break;
                case 9:
                    // StateInfo* st_;
                {
                    // remove old stateInfo
                    {
                        // get list
                        std::vector<StateInfo*> sts;
                        {
                            StateInfo* temp = position->st_;
                            while (temp!=NULL) {
                                sts.push_back(temp);
                                temp = temp->previous;
                            }
                        }
                        // free
                        /*for(int32_t i=0; i<sts.size(); i++){
                            StateInfo* st = sts[i];
                            free(st);
                        }*/
                        position->st_ = NULL;
                    }
                    // find stateInfoNumber
                    int32_t stateInfoNumber = 0;
                    {
                        int32_t size = sizeof(int32_t);
                        if(pointerIndex+size<=length){
                            memcpy(&stateInfoNumber, positionBytes + pointerIndex, size);
                            pointerIndex+=size;
                        }else{
                            printf("length error: st_: %d, %d\n", pointerIndex, length);
                            isParseCorrect = false;
                        }
                    }
                    // parse
                    if(stateInfoNumber>0){
                        // get stateInfo list theo thu tu tu turn thap den turn cao
                        std::vector<StateInfo*> sts;
                        // parse
                        {
                            for (int32_t i = 0; i < stateInfoNumber; i++) {
                                // get stateInfo
                                StateInfo* st = (StateInfo*)calloc(1, sizeof(StateInfo));
                                position->pushStateInfo(st);
                                // parse
                                int32_t stateInfoByteLength = StateInfo::parse (st, positionBytes, length, pointerIndex);
                                if (stateInfoByteLength > 0) {
                                    // add to chess
                                    sts.push_back(st);
                                    // change pointer index
                                    pointerIndex+= stateInfoByteLength;
                                } else {
                                    printf("cannot parse\n");
                                    // free(st);
                                    break;
                                }
                            }
                        }
                        // Dat stateInfo vao vi tri cua position
                        {
                            // vector theo thu tu tu turn thap den turn cao
                            for(int32_t i=0; i<sts.size(); i++){
                                // lay turn cuoi dat vao position
                                if(i==sts.size()-1){
                                    position->st_ = sts[i];
                                }
                                if(i!=0){
                                    sts[i]->previous = sts[i-1];
                                }else{
                                    sts[i]->previous = NULL;
                                }
                            }
                        }
                    }else{
                        printf("error, why stateInfoNumber==0");
                        StateInfo* st = (StateInfo*)calloc(1, sizeof(StateInfo));
                        {
                            // int32_t size = sizeof(StateInfo);
                            // memcpy(&position->st_, &position->startState_, size);
                            st->previous = NULL;
                            position->pushStateInfo(st);
                        }
                        position->st_ = st;
                    }
                }
                    break;
                case 10:
                    // Ply gamePly_;
                {
                    int32_t size = sizeof(int32_t);
                    if(pointerIndex+size<=length){
                        memcpy(&position->gamePly_, positionBytes + pointerIndex, size);
                        pointerIndex+=size;
                    }else{
                        printf("length error: gamePly_ %d, %d\n", pointerIndex, length);
                        isParseCorrect = false;
                    }
                }
                    break;
                case 11:
                    // s64 nodes_;
                {
                    int32_t size = sizeof(s64);
                    if(pointerIndex+size<=length){
                        memcpy(&position->nodes_, positionBytes + pointerIndex, size);
                        pointerIndex+=size;
                    }else{
                        printf("length error: nodes_: %d, %d\n", pointerIndex, length);
                        isParseCorrect = false;
                    }
                }
                    break;
                default:
                {
                    // printf("error, convertByteArrayToPosition: unknown index: %d\n", index);
                    alreadyPassAll = true;
                }
                    break;
            }
            // printf("convert byte array to position: count: %d; %d; %d\n", pointerIndex, index, length);
            index++;
            if (!isParseCorrect) {
                printf("not parse correct\n");
                break;
            }
            if (alreadyPassAll) {
                break;
            }
        }
        // check position ok: if not, correct it
        {
            if(canCorrect){
                // TODO Test
                // position->st_->boardKey = 0;
                // position->st_->handKey = 0;
                
                if(position->myIsOK()!=-1){
#ifdef Android
                    __android_log_print(ANDROID_LOG_ERROR, "NativeCore", "error, convertByteArrayToPosition: position not ok\n");
#else
                    printf("error, convertByteArrayToPosition: position not ok\n");
#endif
                    // find fen
                    char fen[1000];
                    {
                        fen[0] = 0;
                    }
                    position->myToSFEN(fen);
                    // printf("convertPositionToByteArray: correct: fen: %s\n", fen);
                    // find correct positionBytes
                    u8* correctPositionBytes;
                    int32_t correctLength = shogi_makePositionByFen(fen, correctPositionBytes);
                    // convert again
                    Position::parse(position, correctPositionBytes, correctLength, 0, false);
                    // printf("correct position success or not: %d\n", position->isOK());
                    free(correctPositionBytes);
                }else{
                    // printf("position ok\n");
                }
            }
        }
        // return
        if (!isParseCorrect) {
            printf("error parse fail: %d; %d; %d\n", pointerIndex, length, start);
            return -1;
        } else {
            // printf("parse success: %d; %d; %d %d\n", pointerIndex, length, start, (pointerIndex - start));
            return (pointerIndex - start);
        }
    }
}
