/*  BasicTurnTimer class
 *
 *  Copyright (C) 2009  Lee Begg and the Thousand Parsec Project
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
#include <ctime>

#include "timercallback.h"
#include "net.h"
#include "settings.h"
#include "protocol.h"


#include "basicturntimer.h"

BasicTurnTimer::BasicTurnTimer() : TurnTimer(), timer(NULL){
    
}

BasicTurnTimer::~BasicTurnTimer(){
    if(timer != NULL){
        timer->setValid(false);
        delete timer;
    }
}

uint32_t BasicTurnTimer::secondsToEOT() const{
    if(timer != NULL){
        return timer->getExpireTime() - time(NULL);
    }
    return UINT32_NEG_ONE;
}

uint32_t BasicTurnTimer::getTurnLength() const{
    Settings* settings = Settings::getSettings();
  
    uint32_t len = atoi(settings->get("turn_length").c_str());
    return len;
}

void BasicTurnTimer::resetTimer(){
    Settings* settings = Settings::getSettings();
  
    if(timer != NULL){
        timer->setValid(false);
        delete timer;
        timer = NULL;
    }
    
    uint32_t len = atoi(settings->get("turn_length").c_str());
    
    timer = new TimerCallback(this, &BasicTurnTimer::timerFinished, len);
    Network::getNetwork()->addTimer(*timer);
    
    //Alert players
    timerStarted();
}

void BasicTurnTimer::timerFinished(){
    //EOT
    timerExpiredStartEOT();
}
