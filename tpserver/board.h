#ifndef BOARD_H
#define BOARD_H

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

#include <string>
#include <stdint.h>

class Message;
class Frame;

/**
 * Board for posting messages
 */
class Board {

  public:
    /**
     * Default constructor
     *
     * Sets modification time to now, and number of messages and boardid 
     * to zero.
     */
    Board();

    /// Sets board ID and updates modification time
    void setBoardID(int i);
    /// Returns board ID
    int getBoardID();

    /// Sets board name and updates modification time
    void setName(const std::string & nname);
    /// Returns board name
    std::string getName();

    /// Sets board description and updates modification time
    void setDescription(const std::string & ndest);
    /// Returns board description
    std::string getDescription();

    /**
     * Adds a message at given position
     *
     * Passing -1 as position will result in adding a message on top of
     * the board.
     *
     * If message is succesfully added modification time is updated and
     * board is updated.
     */
    void addMessage(Message* msg, int pos);

    /**
     * Removes message at given position
     *
     * @param pos valid message position
     * @returns true if message has been successfuly removed, false otherwise
     */
    bool removeMessage(uint32_t pos);

    /**
     * Packs board information into a frame.
     */
    void packBoard(Frame * frame);

    /**
     * Packs the requested message into the frame
     */
    void packMessage(Frame * frame, uint32_t msgnum);

    /**
     * Returns last modification time
     */
    int64_t getModTime() const;

    /**
     * Get count of messages on board
     */
    uint32_t getNumMessages() const;

    /**
     * Sets the number of messages on board
     *
     * @warning should be used ONLY by the persistence module
     */
    void setNumMessages(uint32_t nnm);
    
    /**
     * Sets the modification time
     *
     * @warning should be used ONLY by the persistence module
     */
    void setModTime(uint64_t nmt);

  private:
    /// Count of messages on the board
    uint32_t message_count;
    /// Board identification number
    int boardid;
    /// Board name
    std::string name;
    /// Board description
    std::string description;
    /// Last modification time
    int64_t mod_time;
};

#endif
