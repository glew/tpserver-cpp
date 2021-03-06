/*  Http listen socket for tpserver-cpp
 *
 *  Copyright (C) 2006  Lee Begg and the Thousand Parsec Project
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "playerhttpconn.h"
#include "logging.h"

#include "httpsocket.h"

HttpSocket::HttpSocket() : ListenSocket(LISTEN){
}

HttpSocket::~HttpSocket(){
}

void HttpSocket::openListen(const std::string& address, const std::string& port){ 
    if(port.length() == 0){
        ListenSocket::openListen(address, "80");
    }else{
        ListenSocket::openListen(address, port);
    }
}

Connection::Ptr HttpSocket::acceptConnection(int fd){
    Logger::getLogger()->info("Accepting new http (tp tunneled over http) connection");

    return Connection::Ptr( new PlayerHttpConnection(fd) );
}
