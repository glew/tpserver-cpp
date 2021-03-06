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

#include <tpserver/common.h>
#include <tpserver/message.h>
#include <tpserver/protocolobject.h>
#include <boost/enable_shared_from_this.hpp>

/**
 * Board for posting messages
 */
class Board : public ProtocolObject, public boost::enable_shared_from_this<Board> {

  public:
    /// typedef for shared pointer
    typedef boost::shared_ptr< Board > Ptr;

    /**
     * Constructor
     *
     * Sets modification time to now, and number of messages to zero.
     */
    Board( uint32_t nid, const std::string& nname, const std::string& ndesc );

    /**
     * Adds a message at given position
     *
     * Passing -1 as position will result in adding a message on top of
     * the board.
     *
     * If message is succesfully added modification time is updated and
     * board is updated.
     */
    void addMessage(Message::Ptr msg, int pos);

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
    void pack(OutputFrame::Ptr frame) const;

    /**
     * Packs the requested message into the frame
     */
    void packMessage(OutputFrame::Ptr frame, uint32_t msgnum);

    /**
     * Get count of messages on board
     */
    uint32_t getNumMessages() const;

    /**
     * Sets the number of messages on board and modification time
     *
     * @warning should be used ONLY by the persistence module
     */
    void setPersistenceData( uint32_t nmessage_count, uint64_t nmod_time );

  private:
    /// Count of messages on the board
    uint32_t message_count;

    /// List of MessageID's belonging to this board
    IdList message_ids;

    /// Retrieves if neccessary message list from persistence
    void retrieveMessageList();

};

#endif
