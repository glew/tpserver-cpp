#ifndef CONSTELLATION_H
#define CONSTELLATION_H
/*  constellation class
 *
 *  Copyright (C) 2008  Ryan Neufeld and the Thousand Parsec Project
 *  Copyright (C) 2007  Tyler Shaub and the Thousand Parsec Project
 *  Copyright (C) 2008  Lee Begg and the Thousand Parsec Project
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

#include "staticobject.h"

class IntegerObjectParam;

namespace RiskRuleset {

class ConstellationType : public StaticObjectType
{
public:
   ConstellationType();
   virtual ~ConstellationType(){};

protected:
   ObjectBehaviour* createObjectBehaviour() const;
};

class Constellation : public StaticObject 
{
public:
   Constellation();
   virtual ~Constellation() {}

   virtual void doOnceATurn();
   virtual int getContainerType();


   void setBonus(int bonus);
   int getBonus() const;

   void setupObject();

};

}

#endif
