/*  Server terminal console for server
 *
 *  Copyright (C) 2003-2005, 2007  Lee Begg and the Thousand Parsec Project
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

#include <iostream>
#include <string.h>
#include <stdio.h>

#include <tprl/rlcommand.h>
#include <tprl/commandcategory.h>
#include <tprl/commandalias.h>
#include <tprl/console.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "logging.h"
#include "net.h"
#include "game.h"
#include "object.h"
#include "settings.h"
#include "pluginmanager.h"
#include "ruleset.h"

#include "tpserver/console.h"

class QuitCommand : public tprl::RLCommand{
    public:
      QuitCommand() : tprl::RLCommand(){
            name = "quit";
            help = "Quit and shutdown the server.";
        }
        void action(const std::string& cmdline){
            Network::getNetwork()->stopMainLoop();
        }
};

class EndTurnCommand : public tprl::RLCommand{
    public:
      EndTurnCommand() : tprl::RLCommand(){
            name = "end";
            help = "End the current turn now and start processing.";
        }
        void action(const std::string& cmdline){
            Logger::getLogger()->info("End Of Turn initiated from console");
            Game::getGame()->doEndOfTurn();
            Game::getGame()->resetEOTTimer();
        }
};

class TurnTimeCommand : public tprl::RLCommand{
    public:
      TurnTimeCommand() : tprl::RLCommand(){
            name = "time";
            help = "The amount of time until the next turn.";
        }
        void action(const std::string& cmdline){
            std::cout << Game::getGame()->secondsToEOT() << " seconds to EOT" << std::endl;
        }
};

class TurnResetCommand : public tprl::RLCommand{
    public:
      TurnResetCommand() : tprl::RLCommand(){
            name = "reset";
            help = "Reset the timer for the next turn.";
        }
        void action(const std::string& cmdline){
            Game::getGame()->resetEOTTimer();
            std::cout << "Reset EOT timer, " << Game::getGame()->secondsToEOT() 
                    << " seconds to EOT" << std::endl;
        }
};

class TurnLengthCommand : public tprl::RLCommand{
    public:
      TurnLengthCommand() : tprl::RLCommand(){
            name = "length";
            help = "Sets the turn length, but doesn't affect the current turn.";
        }
        void action(const std::string& cmdline){
            uint32_t tlen = atoi(cmdline.c_str());
            Game::getGame()->setTurnLength(tlen);
            std::cout << "Setting turn length to " << tlen << " seconds" << std::endl;
        }
};

class TurnNumberCommand : public tprl::RLCommand{
  public:
    TurnNumberCommand() : tprl::RLCommand(){
      name = "number";
      help = "Gets the current turn number.";
    }
    void action(const std::string& cmdline){
      std::cout << "The current turn number is " << Game::getGame()->getTurnNumber() << std::endl;
    }
};

class NetworkStartCommand : public tprl::RLCommand{
    public:
      NetworkStartCommand() : tprl::RLCommand(){
            name = "start";
            help = "Starts the network listeners and accepts connections.";
        }
        void action(const std::string& cmdline){
            Network::getNetwork()->start();
        }
};

class NetworkStopCommand : public tprl::RLCommand{
    public:
      NetworkStopCommand() : tprl::RLCommand(){
            name = "stop";
            help = "Stops the network listeners and drops all connections.";
        }
        void action(const std::string& cmdline){
            Network::getNetwork()->stop();
        }
};

class SettingsSetCommand : public tprl::RLCommand{
    public:
      SettingsSetCommand() : tprl::RLCommand(){
            name = "set";
            help = "Sets a setting.";
        }
        void action(const std::string& cmdline){
            size_t pos1 = cmdline.find(' ');
            Settings::getSettings()->set(cmdline.substr(0, pos1), cmdline.substr(pos1+1));
            std::cout << "Setting value of \"" << cmdline.substr(0, pos1) << "\" to \"" << cmdline.substr(pos1+1) << "\"." << std::endl;
        }
};

class SettingsGetCommand : public tprl::RLCommand{
    public:
      SettingsGetCommand() : tprl::RLCommand(){
            name = "get";
            help = "Gets a setting.";
        }
        void action(const std::string& cmdline){
            std::cout << "Setting \"" << cmdline << "\" is set to \"" << Settings::getSettings()->get(cmdline) << "\"." << std::endl;
        }
};

class PluginLoadCommand : public tprl::RLCommand{
  public:
    PluginLoadCommand() : tprl::RLCommand(){
      name = "load";
      help = "Loads a plugin.";
    }
    void action(const std::string& cmdline){
      if(PluginManager::getPluginManager()->load(cmdline)){
        std::cout << "Plugin \"" << cmdline << "\" was loaded." << std::endl;
      }else{
        std::cout << "Plugin \"" << cmdline << "\" was not loaded." << std::endl;
      }
    }
};

class PluginListCommand : public tprl::RLCommand{
  public:
    PluginListCommand() : tprl::RLCommand(){
      name = "list";
      help = "Lists the plugins that are loaded.";
    }
    void action(const std::string& cmdline){
      std::cout << "These plugins are loaded: " << PluginManager::getPluginManager()->getLoadedLibraryNames() << std::endl;
    }
};

class GameRulesetCommand : public tprl::RLCommand{
  public:
    GameRulesetCommand() : tprl::RLCommand(){
      name = "ruleset";
      help = "Sets the ruleset to be used by ther server.";
    }
    void action(const std::string& cmdline){
      if(PluginManager::getPluginManager()->loadRuleset(cmdline)){
        std::cout << "Ruleset \"" << cmdline << "\" was loaded." << std::endl;
      }else{
        std::cout << "Ruleset \"" << cmdline << "\" was not loaded." << std::endl;
      }
    }
};

class GameTpschemeCommand : public tprl::RLCommand{
  public:
    GameTpschemeCommand() : tprl::RLCommand(){
      name = "tpscheme";
      help = "Sets the tpscheme implementation to be used by ther server.";
    }
    void action(const std::string& cmdline){
      if(PluginManager::getPluginManager()->loadTpScheme(cmdline)){
        std::cout << "TpScheme implementation \"" << cmdline << "\" was loaded." << std::endl;
      }else{
        std::cout << "TpScheme implementation \"" << cmdline << "\" was not loaded." << std::endl;
      }
    }
};

class GamePersistenceCommand : public tprl::RLCommand{
  public:
    GamePersistenceCommand() : tprl::RLCommand(){
      name = "persistence";
      help = "Sets the persistence method to be used by ther server.";
    }
    void action(const std::string& cmdline){
      if(PluginManager::getPluginManager()->loadPersistence(cmdline)){
        std::cout << "Persistence method \"" << cmdline << "\" was loaded." << std::endl;
      }else{
        std::cout << "Persistence method \"" << cmdline << "\" was not loaded." << std::endl;
      }
    }
};

class GameLoadCommand : public tprl::RLCommand{
  public:
    GameLoadCommand() : tprl::RLCommand(){
      name = "load";
      help = "Loads the initial data for the game.";
    }
    void action(const std::string& cmdline){
      Game::getGame()->load();
    }
};

class GameStartCommand : public tprl::RLCommand{
  public:
    GameStartCommand() : tprl::RLCommand(){
      name = "start";
      help = "Starts the game and the end of turn timer.";
    }
    void action(const std::string& cmdline){
      Game::getGame()->start();
    }
};

class StatusCommand : public tprl::RLCommand{
  public:
    StatusCommand() : tprl::RLCommand(){
      name = "status";
      help = "Prints the key info about the server and game.";
    }
    void action(const std::string& cmdline){
      std::cout << "Server: tpserver-cpp" << std::endl;
      std::cout << "Version: " VERSION << std::endl;
      std::cout << "Persistence available: " << ((Game::getGame()->getPersistence() != NULL) ? "yes" : "no") << std::endl;
      Ruleset* rules = Game::getGame()->getRuleset();
      std::cout << "Ruleset Loaded: " << ((rules != NULL) ? "yes" : "no") << std::endl;
      if(rules != NULL){
        std::cout << "Ruleset Name: " << rules->getName() << std::endl;
        std::cout << "Ruleset Version: " << rules->getVersion() << std:: endl;
      }
      std::cout << "Game Loaded: " << ((Game::getGame()->isLoaded()) ? "yes" : "no") << std::endl;
      std::cout << "Game Started: " << ((Game::getGame()->isStarted()) ? "yes" : "no") << std::endl;
      if(Game::getGame()->isStarted()){
        std::cout << "Time to next turn: " << Game::getGame()->secondsToEOT() << std::endl;
        std::cout << "Turn number: " << Game::getGame()->getTurnNumber() << std::endl;
      }
      std::cout << "Network Started: " << ((Network::getNetwork()->isStarted()) ? "yes" : "no") << std::endl;
    }
};

Console *Console::myInstance = NULL;

Console *Console::getConsole()
{
	if (myInstance == NULL)
		myInstance = new Console();
	return myInstance;
}

void Console::open(){
  Network::getNetwork()->addConnection(this);
  Logger::getLogger()->info("Console opened");
  if(console == NULL)
    console = tprl::Console::getConsole();
  console->setCatchSignals(false);
  console->setUseHistory(true);
  console->setCommandSet(&commands);
  console->setPrompt("tpserver-cpp> ");
  console->readLine_nb_start();
  Logger::getLogger()->info("Console ready");
}

void Console::process(){
  console->readLine_nb_inputReady();
}

void Console::redisplay(){
  console->redrawLineForced();
}

void Console::close()
{
  Network::getNetwork()->removeConnection(this);
  console->readLine_nb_stop();
  Logger::getLogger()->info("Console closed");
}


Console::Console() : Connection()
{
	status = 1;
	sockfd = 2;
        console = NULL;

    commands.clear();
    tprl::CommandCategory* cat = new tprl::CommandCategory("turn","Turn management functions");
    cat->add(new EndTurnCommand());
    cat->add(new TurnTimeCommand());
    cat->add(new TurnResetCommand());
    cat->add(new TurnLengthCommand());
    cat->add(new TurnNumberCommand());
    commands.insert(cat);
    
    cat = new tprl::CommandCategory("network", "Network management functions");
    cat->add(new NetworkStartCommand());
    cat->add(new NetworkStopCommand());
    commands.insert(cat);
    
    cat = new tprl::CommandCategory("settings", "Setting control functions");
    cat->add(new SettingsSetCommand());
    cat->add(new SettingsGetCommand());
    commands.insert(cat);
    
    tprl::RLCommand* quit = new QuitCommand();
    commands.insert(quit);
    tprl::CommandAlias* alias = new tprl::CommandAlias("exit");
    alias->setTarget(quit);
    commands.insert(alias);
    
    cat = new tprl::CommandCategory("plugin", "Plugin management functions");
    cat->add(new PluginLoadCommand());
    cat->add(new PluginListCommand());
    commands.insert(cat);

    cat = new tprl::CommandCategory("game", "Game setup and status functions");
    cat->add(new GameRulesetCommand());
    cat->add(new GamePersistenceCommand());
    cat->add(new GameTpschemeCommand());
    cat->add(new GameLoadCommand());
    cat->add(new GameStartCommand());
    commands.insert(cat);
    
    tprl::RLCommand* status = new StatusCommand();
    commands.insert(status);
}

Console::~Console()
{
  if(console != NULL)
    delete console;
}
