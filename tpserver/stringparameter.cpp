/*  StringParameter baseclass
 *
 *  Copyright (C) 2007 Lee Begg and the Thousand Parsec Project
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

#include <stdlib.h>

#include "frame.h"

#include "stringparameter.h"

StringParameter::StringParameter( const std::string& aname, const std::string& adesc ) : OrderParameter(aname,adesc) {
  id = opT_String;
}

StringParameter::~StringParameter(){

}


void StringParameter::packOrderFrame(Frame * f){
  f->packInt(1024);
  f->packString(string);
}

bool StringParameter::unpack(Frame *f){
  if(!f->isEnoughRemaining(8))
    return false;
  f->unpackInt();
  string = f->unpackStdString();
  return true;
}

std::string StringParameter::getString() const{
  return string;
}

void StringParameter::setString(const std::string& rhs){
  string = rhs;
}

