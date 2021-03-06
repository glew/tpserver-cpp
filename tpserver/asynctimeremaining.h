#ifndef ASYNCTIMEREMAINING_H
#define ASYNCTIMEREMAINING_H
/*  AsyncTimeRemaining class
 *
 *  Copyright (C) 2007, 2008  Lee Begg and the Thousand Parsec Project
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

#include <tpserver/outputframe.h>
#include <tpserver/asyncframe.h>

class AsyncTimeRemaining: public AsyncFrame{
  public:
    AsyncTimeRemaining(uint32_t rt, uint32_t r, std::set<playerid_t> waiting);
    virtual ~AsyncTimeRemaining();
    
    virtual bool createFrame(OutputFrame::Ptr f);
    
  private:
    uint32_t rtime;
    uint32_t why;
    std::set<playerid_t> waiting;
};

#endif
