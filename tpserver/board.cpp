/*  Messages boards
 *
 *  Copyright (C) 2004-2005  Lee Begg and the Thousand Parsec Project
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "frame.h"
#include "message.h"
#include "game.h"
#include "boardmanager.h"
#include "persistence.h"
#include "logging.h"

#include "board.h"

Board::Board(uint32_t id, const std::string& nname, const std::string& ndesc) {
  message_count = 0;
  boardid = id;
  name = nname;
  description = ndesc;
  mod_time = time(NULL);
}

int Board::getBoardID() const {
  return boardid;
}

std::string Board::getName() const {
  return name;
}

std::string Board::getDescription() const {
  return description;
}

void Board::addMessage(Message * msg, int pos) {
  retrieveMessageList();
  uint32_t msgid = Game::getGame()->getBoardManager()->addMessage(msg);
  if (msgid == UINT32_NEG_ONE)
  {
    // error report?
    return;
  }
  if (pos == -1) {
    message_ids.push_back(msgid);
  } else {
    IdList::iterator inspos = message_ids.begin();
    advance(inspos, pos);
    message_ids.insert(inspos, msgid);
  }
  message_count = message_ids.size();
  Game::getGame()->getPersistence()->saveMessageList(boardid, message_ids);
  Game::getGame()->getPersistence()->updateBoard(this);
  mod_time = time(NULL);
}

bool Board::removeMessage(uint32_t pos){
  retrieveMessageList();
  if (pos >= message_ids.size()) {
    return false;
  }
  IdList::iterator itpos = message_ids.begin();
  advance(itpos, pos);
  uint32_t msgid = *itpos;
  message_ids.erase(itpos);
  message_count = message_ids.size();

  bool result = true;
  result = result && Game::getGame()->getBoardManager()->removeMessage(msgid);
  result = result && Game::getGame()->getPersistence()->saveMessageList(boardid, message_ids);
  Game::getGame()->getPersistence()->updateBoard(this);

  if (result) {
    mod_time = time(NULL);
  }
  return true;
}

void Board::packBoard(Frame * frame){
  frame->setType(ft02_Board);
  frame->packInt(boardid);
  frame->packString(name.c_str());
  frame->packString(description.c_str());
  frame->packInt(message_count);
  frame->packInt64(mod_time);
}

void Board::packMessage(Frame * frame, uint32_t msgnum) {
  if (msgnum < message_count) {
    Message* message = NULL;
    retrieveMessageList();
    if (msgnum < message_ids.size()) {
      IdList::iterator itpos = message_ids.begin();
      advance(itpos, msgnum);
      message = Game::getGame()->getBoardManager()->getMessage(*itpos);
    }
    if (message != NULL) {
      frame->setType(ft02_Message);
      frame->packInt(boardid);
      frame->packInt(msgnum);
      message->pack(frame);
    } else {
      frame->createFailFrame(fec_TempUnavailable, "Could not get Message at this time");
      Logger::getLogger()->warning("Board has messages but persistence didn't get it");
      Logger::getLogger()->warning("POSSIBLE DATABASE INCONSISTANCE");
    }
  } else {
    frame->createFailFrame(fec_NonExistant, "No such Message on board");
  }
}

int64_t Board::getModTime() const{
  return mod_time;
}

uint32_t Board::getNumMessages() const{
  return message_count;
}

void Board::setPersistenceData( uint32_t nmessage_count, uint64_t nmod_time ) {
  message_count = nmessage_count;
  mod_time = nmod_time;
}

void Board::retrieveMessageList() {
  if (message_ids.empty()) {
    message_ids = Game::getGame()->getPersistence()->retrieveMessageList(boardid);
  }
}
