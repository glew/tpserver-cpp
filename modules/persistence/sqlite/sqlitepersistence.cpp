/* 
 * File:   sqlitepersistence.cpp
 * Author: greg
 * 
 * Created on June 17, 2010, 10:54 AM
 */

#include <sstream>

#include <tpserver/logging.h>
#include <tpserver/settings.h>
#include <tpserver/game.h>
#include <tpserver/ordermanager.h>
#include <tpserver/objecttypemanager.h>

#include <tpserver/object.h>
#include <tpserver/orderqueue.h>
#include <tpserver/order.h>
#include <tpserver/orderparameter.h>
#include <tpserver/orderparameters.h>
#include <tpserver/board.h>
#include <tpserver/message.h>
#include <tpserver/resourcedescription.h>
#include <tpserver/player.h>
#include <tpserver/playerview.h>
#include <tpserver/category.h>
#include <tpserver/design.h>
#include <tpserver/propertyvalue.h>
#include <tpserver/component.h>
#include <tpserver/property.h>

#include <tpserver/objectview.h>
#include <tpserver/designview.h>
#include <tpserver/componentview.h>

//Objectparameters
#include <tpserver/position3dobjectparam.h>
#include <tpserver/velocity3dobjectparam.h>
#include <tpserver/orderqueueobjectparam.h>
#include <tpserver/resourcelistobjectparam.h>
#include <tpserver/referenceobjectparam.h>
#include <tpserver/refquantitylistobjectparam.h>
#include <tpserver/integerobjectparam.h>
#include <tpserver/sizeobjectparam.h>
#include <tpserver/mediaobjectparam.h>

#include <sqlite3.h>

#include "sqlitepersistence.h"

extern "C" {
#define tp_init libtpsqlite_LTX_tp_init
  bool tp_init(){
    return Game::getGame()->setPersistence(new SqlitePersistence());
  }
}

class SqliteException : public std::exception {
    public:
        // CHECK: passing the error message
        SqliteException( char **errmsg, const std::string& error ) {
            errorstr = error + " " + std::string(errmsg);
            Logger::getLogger()->error( ("Sqlite : "+errorstr).c_str() );
        }

        ~SqliteException() throw() {
        }

        const char* what() const throw() {
            return errorstr.c_str();
        }

    private:
        std::string errorstr;
};



SqlitePersistence::SqlitePersistence() : db(NULL), db_err(NULL) {
}

SqlitePersistence::~SqlitePersistence() {
    if(db! = NULL)
        shutdown();
}

bool SqlitePersistence::init() {
    // CHECK: I think the only thing needed to open a connection to a SQLite db is the db name (location)
    
    lock();

    // CHECK: need to look into finding whether persistence is running.
    if(db != NULL){
        Logger::getLogger()->warning("Persistence already running");
        unlock();
        return false;
    }
    Settings* conf = Settings::getSettings();
    
    const char* host = NULL;
    std::string shost = conf->get("sqlite_host");
    if(shost.length() != 0)
        host = shost.c_str();
    const char* user = NULL;
    std::string suser = conf->get("sqlite_user");
    if(suser.length() != 0)
        user = suser.c_str();
    const char* pass = NULL;
    std::string spass = conf->get("sqlite_pass");
    if(spass.length() != 0)
        pass = spass.c_str();
    const char* database = NULL;
    std::string sdb = conf->get("sqlite_db");
    if(sdb.length() != 0)
        database = sdb.c_str();
    else{
        Logger::getLogger()->error("sqlite database name not specified");
        // TODO: Look into conn and its relevence to sqlite from mysql
        //sqlite3_close(db) // but db was never opened
        db = NULL;
        unlock();
        return false;
    }
    
    const char* sock = NULL;
    std::string ssock = conf->get("sqlite_socket");
    if(ssock.length() != 0)
        sock = ssock.c_str();
    int port = atoi(conf->get("sqlite_port").c_str());
//    if(mysql_real_connect(conn, host, user, pass, db, port, sock, CLIENT_FOUND_ROWS) == NULL){
//        Logger::getLogger()->error("Could not connect to sql");
//        conn = NULL;
//        unlock();
//        return false;
//    }


    if (sqlite3_open(database, &db)){
        Logger::getLogger()->error("Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        unlock();
        return false;
    }
    
    // check for tables, create if necessary
    //char* db_err;
    sqlite3_exec(db, "SELECT * FROM tableversion;", NULL, 0, &db_err);
    if(db_err != NULL){
        // create tables
        Logger::getLogger()->info("Creating database tables");
        try{
            sqlite3_exec(db, "CREATE TABLE tableversion ( tableid INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY, "
                "name VARCHAR(50) NOT NULL UNIQUE, version INT UNSIGNED NOT NULL);", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db, "INSERT INTO tableversion VALUES (NULL, 'tableversion', 1), (NULL, 'gameinfo', 1), "
            "(NULL, 'object', 0), (NULL, 'objectparamposition', 0), (NULL, 'objectparamvelocity', 0), "
            "(NULL, 'objectparamorderqueue', 0), (NULL, 'objectparamresourcelist', 0), "
            "(NULL, 'objectparamreference', 0), (NULL, 'objectparamrefquantitylist', 0), "
            "(NULL, 'objectparaminteger', 0), (NULL, 'objectparamsize', 0), (NULL, 'objectparammedia', 0), "
            "(NULL, 'orderqueue', 0), (NULL, 'orderqueueowner', 0), (NULL, 'orderqueueallowedtype', 0), "
            "(NULL, 'ordertype', 0), (NULL, 'orderresource', 0), (NULL, 'orderslot', 0), "
            "(NULL, 'orderparamspace', 0), (NULL, 'orderparamobject', 0), "
            "(NULL, 'orderparamstring', 0), (NULL, 'orderparamtime', 0), "
            "(NULL, 'orderparamlist', 0), "
            "(NULL, 'board', 0), (NULL, 'message', 0), (NULL, 'messagereference', 0), (NULL, 'messageslot', 0), "
            "(NULL, 'player', 0), (NULL, 'playerscore', 0), (NULL, 'playerdesignview', 0), (NULL, 'playerdesignviewcomp', 0), "
            "(NULL, 'playerdesignviewprop', 0), (NULL, 'playerdesignusable', 0), (NULL, 'playercomponentview', 0), "
            "(NULL, 'playercomponentviewcat', 0), (NULL, 'playercomponentviewproperty', 0), "
            "(NULL, 'playercomponentusable', 0), (NULL, 'playerobjectview', 0), (NULL, 'playerobjectowned', 0), "
            "(NULL, 'category', 0), (NULL, 'design',0), (NULL, 'designcomponent', 0), (NULL, 'designproperty', 0), "
            "(NULL, 'component', 0), (NULL, 'componentcat', 0), (NULL, 'componentproperty', 0), "
            "(NULL, 'property', 0), (NULL, 'propertycat', 0), (NULL, 'resourcedesc', 0);", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE gameinfo (metakey VARCHAR(50) NOT NULL, ctime BIGINT UNSIGNED NOT NULL PRIMARY KEY, turnnum INT UNSIGNED NOT NULL, turnname VARCHAR(50) NOT NULL);", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE object (objectid INT UNSIGNED NOT NULL, turnnum INT UNSIGNED NOT NULL, alive TINYINT UNSIGNED NOT NULL, type INT UNSIGNED NOT NULL, " 
                    "name TEXT NOT NULL, description TEXT NOT NULL, parentid INT UNSIGNED NOT NULL, modtime BIGINT UNSIGNED NOT NULL, PRIMARY KEY(objectid, turnnum));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE objectparamposition (objectid INT UNSIGNED NOT NULL, turn INT UNSIGNED NOT NULL, playerid INT UNSIGNED NOT NULL, paramgroupid INT UNSIGNED NOT NULL, paramgrouppos INT UNSIGNED NOT NULL, posx BIGINT NOT NULL, posy BIGINT NOT NULL, posz BIGINT NOT NULL, relative INT UNSIGNED NOT NULL, PRIMARY KEY(objectid, turn, playerid, paramgroupid, paramgrouppos));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE objectparamvelocity (objectid INT UNSIGNED NOT NULL, turn INT UNSIGNED NOT NULL, playerid INT UNSIGNED NOT NULL, paramgroupid INT UNSIGNED NOT NULL, paramgrouppos INT UNSIGNED NOT NULL, velx BIGINT NOT NULL, vely BIGINT NOT NULL, velz BIGINT NOT NULL, relative INT UNSIGNED NOT NULL, PRIMARY KEY(objectid, turn, playerid, paramgroupid, paramgrouppos));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE objectparamorderqueue (objectid INT UNSIGNED NOT NULL, turn INT UNSIGNED NOT NULL, playerid INT UNSIGNED NOT NULL, paramgroupid INT UNSIGNED NOT NULL, paramgrouppos INT UNSIGNED NOT NULL, queueid INT UNSIGNED NOT NULL, PRIMARY KEY(objectid, turn, playerid, paramgroupid, paramgrouppos));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE objectparamresourcelist (objectid INT UNSIGNED NOT NULL, turn INT UNSIGNED NOT NULL, playerid INT UNSIGNED NOT NULL, paramgroupid INT UNSIGNED NOT NULL, paramgrouppos INT UNSIGNED NOT NULL, resid INT UNSIGNED NOT NULL, available INT UNSIGNED NOT NULL, possible INT UNSIGNED NOT NULL, PRIMARY KEY(objectid, turn, playerid, paramgroupid, paramgrouppos, resid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE objectparamreference (objectid INT UNSIGNED NOT NULL, turn INT UNSIGNED NOT NULL, playerid INT UNSIGNED NOT NULL, paramgroupid INT UNSIGNED NOT NULL, paramgrouppos INT UNSIGNED NOT NULL, reftype INT NOT NULL, refval INT UNSIGNED NOT NULL, PRIMARY KEY(objectid, turn, playerid, paramgroupid, paramgrouppos));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE objectparamrefquantitylist (objectid INT UNSIGNED NOT NULL, turn INT UNSIGNED NOT NULL, playerid INT UNSIGNED NOT NULL, paramgroupid INT UNSIGNED NOT NULL, paramgrouppos INT UNSIGNED NOT NULL, reftype INT NOT NULL, refid INT UNSIGNED NOT NULL, quant INT UNSIGNED NOT NULL, PRIMARY KEY(objectid, turn, playerid, paramgroupid, paramgrouppos, reftype, refid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE objectparaminteger (objectid INT UNSIGNED NOT NULL, turn INT UNSIGNED NOT NULL, playerid INT UNSIGNED NOT NULL, paramgroupid INT UNSIGNED NOT NULL, paramgrouppos INT UNSIGNED NOT NULL, val INT UNSIGNED NOT NULL, PRIMARY KEY(objectid, turn, playerid, paramgroupid, paramgrouppos));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE objectparamsize (objectid INT UNSIGNED NOT NULL, turn INT UNSIGNED NOT NULL, playerid INT UNSIGNED NOT NULL, paramgroupid INT UNSIGNED NOT NULL, paramgrouppos INT UNSIGNED NOT NULL, size BIGINT UNSIGNED NOT NULL, PRIMARY KEY(objectid, turn, playerid, paramgroupid, paramgrouppos));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE objectparammedia (objectid INT UNSIGNED NOT NULL, turn INT UNSIGNED NOT NULL, playerid INT UNSIGNED NOT NULL, paramgroupid INT UNSIGNED NOT NULL, paramgrouppos INT UNSIGNED NOT NULL, url VARCHAR(200) NOT NULL, PRIMARY KEY(objectid, turn, playerid, paramgroupid, paramgrouppos));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE orderqueue (queueid INT UNSIGNED NOT NULL PRIMARY KEY, objectid INT UNSIGNED NOT NULL, active TINYINT NOT NULL, repeating TINYINT NOT NULL, modtime BIGINT UNSIGNED NOT NULL);", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE orderqueueowner (queueid INT UNSIGNED NOT NULL, playerid INT UNSIGNED NOT NULL, PRIMARY KEY(queueid, playerid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE orderqueueallowedtype (queueid INT UNSIGNED NOT NULL, ordertype INT UNSIGNED NOT NULL, PRIMARY KEY(queueid, ordertype));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE ordertype (queueid INT UNSIGNED NOT NULL, orderid INT UNSIGNED NOT NULL, type INT UNSIGNED NOT NULL, turns INT UNSIGNED NOT NULL, PRIMARY KEY(queueid, orderid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE orderresource (queueid INT UNSIGNED NOT NULL, orderid INT UNSIGNED NOT NULL, resourceid INT UNSIGNED NOT NULL, amount INT UNSIGNED NOT NULL, PRIMARY KEY (queueid, orderid, resourceid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE orderparamspace (queueid INT UNSIGNED NOT NULL, orderid INT UNSIGNED NOT NULL, position INT UNSIGNED NOT NULL, posx BIGINT NOT NULL, posy BIGINT NOT NULL, posz BIGINT NOT NULL, PRIMARY KEY (queueid, orderid, position));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE orderparamobject (queueid INT UNSIGNED NOT NULL, orderid INT UNSIGNED NOT NULL, position INT UNSIGNED NOT NULL, objectid INT UNSIGNED NOT NULL, PRIMARY KEY (queueid, orderid, position));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE orderparamstring (queueid INT UNSIGNED NOT NULL, orderid INT UNSIGNED NOT NULL, position INT UNSIGNED NOT NULL, thestring TEXT NOT NULL, PRIMARY KEY (queueid, orderid, position));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE orderparamtime (queueid INT UNSIGNED NOT NULL, orderid INT UNSIGNED NOT NULL, position INT UNSIGNED NOT NULL, turns INT UNSIGNED NOT NULL, PRIMARY KEY (queueid, orderid, position));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE orderparamlist (queueid INT UNSIGNED NOT NULL, orderid INT UNSIGNED NOT NULL, position INT UNSIGNED NOT NULL, listid INT UNSIGNED NOT NULL, amount INT UNSIGNED NOT NULL, PRIMARY KEY (queueid, orderid, position, listid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE orderslot (queueid INT UNSIGNED NOT NULL, slot INT UNSIGNED NOT NULL, "
                    "orderid INT UNSIGNED NOT NULL, PRIMARY KEY (queueid, slot, orderid), UNIQUE (queueid, slot), UNIQUE(queueid, orderid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE board (boardid INT UNSIGNED NOT NULL PRIMARY KEY, "
                    "name TEXT NOT NULL, description TEXT NOT NULL, nummessages INT UNSIGNED NOT NULL, "
                    "modtime BIGINT UNSIGNED NOT NULL);", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE message (messageid INT UNSIGNED NOT NULL PRIMARY KEY, subject TEXT NOT NULL, "
                    "body TEXT NOT NULL, turn INT UNSIGNED NOT NULL);", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE messagereference (messageid INT UNSIGNED NOT NULL, type INT NOT NULL, "
                    "refid INT UNSIGNED NOT NULL, PRIMARY KEY (messageid, type, refid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE messageslot (boardid INT UNSIGNED NOT NULL, slot INT UNSIGNED NOT NULL, "
                    "messageid INT UNSIGNED NOT NULL UNIQUE, PRIMARY KEY (boardid, slot));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE player (playerid INT UNSIGNED NOT NULL PRIMARY KEY, name TEXT NOT NULL, "
                    "password TEXT NOT NULL, email TEXT NOT NULL, comment TEXT NOT NULL, boardid INT UNSIGNED NOT NULL, "
                    "alive TINYINT UNSIGNED NOT NULL, modtime BIGINT NOT NULL, UNIQUE (name(255)));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE playerscore (playerid INT UNSIGNED NOT NULL, scoreid INT UNSIGNED NOT NULL, scoreval INT UNSIGNED NOT NULL, PRIMARY KEY(playerid, scoreid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE playerdesignview (playerid INT UNSIGNED NOT NULL, designid INT UNSIGNED NOT NULL, "
                    "completelyvisible TINYINT NOT NULL, namevisible TINYINT NOT NULL,"
                    "visname TEXT NOT NULL, descvisible TINYINT NOT NULL, visdesc TEXT NOT NULL, existvisible TINYINT NOT NULL, visexist INT UNSIGNED NOT NULL, ownervisible TINYINT NOT NULL, visowner INT UNSIGNED NOT NULL, modtime BIGINT UNSIGNED NOT NULL, PRIMARY KEY (playerid, designid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE playerdesignviewcomp (playerid INT UNSIGNED NOT NULL, designid INT UNSIGNED NOT NULL, "
                    "componentid INT UNSIGNED NOT NULL, quantity INT UNSIGNED NOT NULL, "
                    "PRIMARY KEY (playerid, designid, componentid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE playerdesignviewprop (playerid INT UNSIGNED NOT NULL, designid INT UNSIGNED NOT NULL, "
                    "propertyid INT UNSIGNED NOT NULL, value TEXT NOT NULL, "
                    "PRIMARY KEY (playerid, designid, propertyid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE playerdesignusable (playerid INT UNSIGNED NOT NULL, designid INT UNSIGNED NOT NULL, "
                    "PRIMARY KEY (playerid, designid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE playercomponentview (playerid INT UNSIGNED NOT NULL, componentid INT UNSIGNED NOT NULL, "
                    "completelyvisible TINYINT NOT NULL, namevisible TINYINT NOT NULL, visname TEXT NOT NULL, "
                    "descvisible TINYINT NOT NULL, visdesc TEXT NOT NULL, reqfuncvis TINYINT NOT NULL, "
                    "modtime BIGINT UNSIGNED NOT NULL, PRIMARY KEY (playerid, componentid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE playercomponentviewcat (playerid INT UNSIGNED NOT NULL, componentid INT UNSIGNED NOT NULL, "
                    "categoryid INT UNSIGNED NOT NULL, PRIMARY KEY (playerid, componentid, categoryid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE playercomponentviewproperty (playerid INT UNSIGNED NOT NULL, componentid INT UNSIGNED NOT NULL, "
                    "propertyid INT UNSIGNED NOT NULL, PRIMARY KEY (playerid, componentid, propertyid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE playercomponentusable (playerid INT UNSIGNED NOT NULL, componentid INT UNSIGNED NOT NULL, "
                    "PRIMARY KEY (playerid, componentid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE playerobjectview (playerid INT UNSIGNED NOT NULL, objectid INT UNSIGNED NOT NULL, "
                    "turnnum INT UNSIGNED NOT NULL, completelyvisible TINYINT NOT NULL, gone TINYINT NOT NULL, "
                    "namevisible TINYINT NOT NULL, visname TEXT NOT NULL, descvisible TINYINT NOT NULL, "
                    "visdesc TEXT NOT NULL, "
                    "modtime BIGINT UNSIGNED NOT NULL, PRIMARY KEY (playerid, objectid, turnnum));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE playerobjectowned (playerid INT UNSIGNED NOT NULL, objectid INT UNSIGNED NOT NULL, "
                    "PRIMARY KEY (playerid, objectid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE category (categoryid INT UNSIGNED NOT NULL PRIMARY KEY, name TEXT NOT NULL, "
                    "description TEXT NOT NULL, modtime BIGINT UNSIGNED NOT NULL);", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE design (designid INT UNSIGNED NOT NULL PRIMARY KEY, categoryid INT UNSIGNED NOT NULL,"
                    "name TEXT NOT NULL, description TEXT NOT NULL, owner INT NOT NULL, inuse INT UNSIGNED NOT NULL,"
                    "numexist INT UNSIGNED NOT NULL, valid TINYINT NOT NULL, feedback TEXT NOT NULL, modtime BIGINT UNSIGNED NOT NULL);", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE designcomponent (designid INT UNSIGNED NOT NULL, componentid INT UNSIGNED NOT NULL, "
                    "count INT UNSIGNED NOT NULL, PRIMARY KEY (designid, componentid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE designproperty (designid INT UNSIGNED NOT NULL, propertyid INT UNSIGNED NOT NULL, "
                    "value DOUBLE  NOT NULL, displaystring TEXT NOT NULL, PRIMARY KEY (designid, propertyid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE component (componentid INT UNSIGNED NOT NULL PRIMARY KEY, "
                    "name TEXT NOT NULL, description TEXT NOT NULL, tpclrequiresfunc TEXT NOT NULL, modtime BIGINT UNSIGNED NOT NULL);", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE componentcat (componentid INT UNSIGNED NOT NULL, categoryid INT UNSIGNED NOT NULL, PRIMARY KEY (componentid, categoryid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE componentproperty (componentid INT UNSIGNED NOT NULL, propertyid INT UNSIGNED NOT NULL, "
                    "tpclvaluefunc TEXT NOT NULL, PRIMARY KEY (componentid, propertyid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE property (propertyid INT UNSIGNED NOT NULL PRIMARY KEY,"
                    "rank INT UNSIGNED NOT NULL, name TEXT NOT NULL, displayname TEXT NOT NULL, description TEXT NOT NULL, "
                    "tpcldisplayfunc TEXT NOT NULL, tpclrequiresfunc TEXT NOT NULL, modtime BIGINT UNSIGNED NOT NULL);", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE propertycat (propertyid INT UNSIGNED NOT NULL, categoryid INT UNSIGNED NOT NULL, PRIMARY KEY (propertyid, categoryid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE resourcedesc (resourcetype INT UNSIGNED NOT NULL PRIMARY KEY, name_sig TEXT NOT NULL,"
                    "name_plur TEXT NOT NULL, unit_sig TEXT NOT NULL, uint_plur TEXT NOT NULL, description TEXT NOT NULL, "
                    "mass INT UNSIGNED NOT NULL, volume INT UNSIGNED NOT NULL, modtime BIGINT UNSIGNED NOT NULL);", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
        }catch(std::exception e){
            Logger::getLogger()->error("Sqlite creating tables: %s", db_err);
            Logger::getLogger()->error("You may need to delete the tables and start again");
            sqlite3_close(db);
            unlock();
            return false;
        }
    }else{
        // check for tables to be updates.
        sqlite3_stmt stmt;
        sqlite3_prepare_v2(db, "SELECT * FROM tableversion;", -1, &stmt, NULL);
        if (sqlite3_step(stmt) == SQLITE_DONE){
            Logger::getLogger()->error("Sqlite: table versions query result error");
            Logger::getLogger()->error("You may need to delete the tables and start again");
            sqlite3_close(db);
            unlock();
            return false;
        }
        //Possible central table updates
        sqlite3_finalize(stmt);

        try{
            if(getTableVersion("tableversion") == 0){
                Logger::getLogger()->error("Old database format detected.");
                Logger::getLogger()->error("Incompatable old table formats and missing tables detected.");
                Logger::getLogger()->error("Changes to most stored clcasses means tehre is no way to update from your current database to the newer format");
                Logger::getLogger()->error("I cannot stress this enough: Please shutdown your game, delete the contents of the database and start again. Sorry");
                Logger::getLogger()->error("Sqlite persistence NOT STARTED");
                return false;
            }
            if(getTableVersion("gameinfo") == 0){
                sqlite3_exec(db,"ALTER TABLE gameinfo ADD COLUMN turnname VARCHAR(50) NOT NULL;", NULL, 0, &db_err);
                if(db_err != NULL){
                    Logger::getLogger()->error("Can't alter gameinfo table, please reset the database");
                    return false;
                }
            }
        }catch(std::exception e){
        }

    }

    unlock();

    if(sqlite3_threadsafe()){
        Logger::getLogger()->debug("Sqlite is thread safe");
    }else{
        Logger::getLogger()->debug("Sqlite is NOT thread safe");
    }

    return true;
}

void SqlitePersistence::shutdown(){
    lock();
    if(db != NULL){
        sqlite3_close(db);
        db = NULL;
    }
    unlock();
}

bool SqlitePersistence::saveGameInfo(){
    lock();
    char* db_err;
    sqlite3_exec(db, "DELETE FROM gameinfo;", NULL, 0, &db_err);
    unlock();
    try{
        std::ostringstream querybuilder;
        Game* game = Game::getGame();
        querybuilder << "INSERT INTO gameinfo VALUES ('" << addslashes(game->getKey()) << "', ";
        querybuilder << game->getGameStartTime() << ", " << game->getTurnNumber();
        querybuilder << ", '" << game->getTurnName() << "');";
        sqlite3_exec(db, querybuilder.str() );
    } catch (SqliteException& e ) {
        return false;
    }
    return true;
}

bool SqlitePersistence::retrieveGameInfo(){
    try {
        SqliteQuery query(db, "SELECT * FROM gameinfo;" );
        query.nextRow();
        Game* game = Game::getGame();
        game->setKey(query.get(0));
        game->setGameStartTime(query.getU64(1));
        game->setTurnNumber(query.getInt(2));
        game->setTurnName(query.get(3));
    } catch (SqliteException& e) { return false; }
    return true;
}

bool SqlitePersistence::saveObject(IGObject::Ptr ob){
    try {
        std::ostringstream querybuilder;
        uint32_t turn = Game::getGame()->getTurnNumber();
        uint32_t obid = ob->getID();

        querybuilder << "DELETE FROM object WHERE objetid = " << obid << " AND turnnum = " << turn << ";";
        singleQuery( querybuilder.str() );

        querybuilder.str("");
        querybuilder << "INSERT INTO object VALUES (" << obid << ", " << turn << ", ";
        querybuilder << (ob->isAlive() ? 1 : 0) << ", " << ob->getType() << ", ";
        querybuilder << << "'" << addslashes(ob->getName()) << "', '" << addslashes(ob->getDescription()) << "', ";
        querybuilder << ob->getParent() << ", " << ob->getModTime() << ");";
        singleQuery( querybuilder.str() );

        bool rtv = true;
        //store type-specific information
        if(ob->isAlive()){
            try{
                ObjectParameterGroup::Map groups = ob->getParameterGroups();
                for(ObjectParameterGroup::Map::iterator itcurr = groups.begin();
                        itcurr != groups.end(); ++itcurr){
                    ObjectParameterGroup::ParameterList params = itcurr->second->getParameters();
                    uint32_t ppos = 0;
                    for(ObjectParameterGroup::ParameterList::iterator paramcurr = params.begin();
                            paramcurr != params.end(); ++paramcurr){
                        ObjectParameter* parameter = *(paramcurr);
                        switch(parameter->getType()){
                            case obpT_Position_3D:
                                updatePosition3dObjectParam(obid, turn, 0, itcurr->first, ppos, static_cast<Position3dObjectParam*>(parameter));
                                break;
                            case obpT_Velocity:
                                updateVelocity3dObjectParam(obid, turn, 0, itcurr->first, ppos, static_cast<Velocity3dObjectParam*>(parameter));
                                break;
                            case obpT_Order_Queue:
                                updateOrderQueueObjectParam(obid, turn, 0, itcurr->first, ppos, static_cast<OrderQueueObjectParam*>(parameter));
                                break;
                            case obpT_Resource_List:
                                updateResourceListObjectParam(obid, turn, 0, itcurr->first, ppos, static_cast<ResourceListObjectParam*>(parameter));
                                break;
                            case obpT_Reference:
                                updateReferenceObjectParam(obid, turn, 0, itcurr->first, ppos, static_cast<ReferenceObjectParam*>(parameter));
                                break;
                            case obpT_Reference_Quantity_List:
                                updateRefQuantityListObjectParam(obid, turn, 0, itcurr->first, ppos, static_cast<RefQuantityListObjectParam*>(parameter));
                                break;
                            case obpT_Integer:
                                updateIntegerObjectParam(obid, turn, 0, itcurr->first, ppos, static_cast<IntegerObjectParam*>(parameter));
                                break;
                            case obpT_Size:
                                updateSizeObjectParam(obid, turn, 0, itcurr->first, ppos, static_cast<SizeObjectParam*>(parameter));
                                break;
                            case obpT_Media:
                                updateMediaObjectParam(obid, turn, 0, itcurr->first, ppos, static_cast<MediaObjectParam*>(parameter));
                                break;
                            default:
                                Logger::getLogger()->error("Unknown ObjectParameter type %d", parameter->getType());
                                throw std::exception();
                                break;
                        }
                        ppos++;
                    }
                }
            } catch (std::exception& e){
                rtv = false;
            }
        }
        ob->setIsDirty(!rtv);

        return rtv;
    } catch ( SqliteException& e ){
        ob->setIsDirty(true);
        return false;
    }
    return true;
}

IGObject::Ptr SqlitePersistence::retrieveObject(uint32_t obid){
    try{
        std::ostringstream querybuilder;
        uint32_t turn = Game::getGame()->getTurnNumber();

        querybuilder << "SELECT * FROM object WHERE objectid = " << obid << " AND turnnum <= " << turn;
        querybuilder << " ORDER BY turnnum DESC LIMIT 1;";
        SqliteQuery query( db, querybuilder.str() );
        query.nextRow();

        IGObject::Ptr object( new IGObject(obid) );

        Game::getGame()->getObjectTypeManager()->setupObject(object, query.getInt(3));
        object->setIsAlive(query.getInt(2) == 1);
        object->setName(query.get(4));
        object->setDescription(query.get(5));
        object->setParent(query.getInt(6));

        //children object ids
        querybuilder.str("");
        // CHECK: object.objectid
        querybuilder << "SELECT object.objectid FROM object JOIN (SELECT objectid, MAX(turnnum) AS maxturnnum FROM object WHERE turnnum <= " << turn << " GROUP BY objectid) AS maxx ON (object.objectid=maxx.objectid AND object.turnnum = maxx.maxturnnum) WHERE parentid = " << obid << " ;";
        SqliteQuery cquery(db, querybuilder.str() );

        while(cquery.nextRow()){
            uint32_t childid = cquery.getInt(0);
            DEBUG("childid: %d", childid);
            if(childid != object->getID())
                object->addContainedObject(childid);
        }
        DEBUG("num children: %d", object->getContainedObjects().size());

        //fetch type-specific information
        if(object->isAlive()){
            try{
                std::map<uint32_t, ObjectParameterGroup::Ptr> groups = object->getParameterGroups();
                for(std::map<uint32_t, ObjectParameterGroup::Ptr>::iterator itcurr = groups.begin();
                        itcurr != groups.end(); ++itcurr){
                    ObjectParameterGroup::ParameterList params = itcurr->second->getParameters();
                    uint32_t ppos = 0;
                    for(ObjectParameterGroup::ParameterList::iterator paramcurr = params.begin();
                            paramcurr != params.end(); ++paramcurr){
                        ObjectParameter* parameter = *(paramcurr);
                        switch(parameter->getType()){
                            case obpT_Position_3D:
                                retrievePosition3dObjectParam(obid, turn, 0, itcurr->first, ppos, static_cast<Position3dObjectParam*>(parameter));
                                break;
                            case obpT_Velocity:
                                retrieveVelocity3dObjectParam(obid, turn, 0, itcurr->first, ppos, static_cast<Velocity3dObjectParam*>(parameter));
                                break;
                            case obpT_Order_Queue:
                                retrieveOrderQueueObjectParam(obid, turn, 0, itcurr->first, ppos, static_cast<OrderQueueObjectParam*>(parameter));
                                break;
                            case obpT_Resource_List:
                                retrieveResourceListObjectParam(obid, turn, 0, itcurr->first, ppos, static_cast<ResourceListObjectParam*>(parameter));
                                break;
                            case obpT_Reference:
                                retrieveReferenceObjectParam(obid, turn, 0, itcurr->first, ppos, static_cast<ReferenceObjectParam*>(parameter));
                                break;
                            case obpT_Reference_Quantity_List:
                                retrieveRefQuantityListObjectParam(obid, turn, 0, itcurr->first, ppos, static_cast<RefQuantityListObjectParam*>(parameter));
                                break;
                            case obpT_Integer:
                                retrieveIntegerObjectParam(obid, turn, 0, itcurr->first, ppos, static_cast<IntegerObjectParam*>(parameter));
                                break;
                            case obpT_Size:
                                retrieveSizeObjectParam(obid, turn, 0, itcurr->first, ppos, static_cast<SizeObjectParam*>(parameter));
                                break;
                            case obpT_Media:
                                retrieveMediaObjectParam(obid, turn, 0, itcurr->first, ppos, static_cast<MediaObjectParam*>(parameter));
                                break;
                            default:
                                Logger::getLogger()->error("Unknown ObjectParameter type %d", parameter->getType());
                                throw std::exception();
                                break;
                        }
                        ppos++;
                    }
                }
            } catch (std::exception& e){
                object.reset();
                return object;
            }
        }

        object->setModTime( query.getU64(7));
        object->setIsDirty(false);

        return object;
    } catch (SqliteException& e ) {
        return IGObject::Ptr();
    }
}

uint32_t SqlitePersistence::getMaxObjectId(){
    try{
        return valueQuery( "SELECT MAX(objectid) from object;");
    } catch (SqliteException& e) {
        return 0;
    }
}

IdSet SqlitePersistence::getObjectIds(){
    try {
        SqliteQuery query(db, "SELECT objectid FROM object;");
        IdSet vis;
        while (query.nextRow() ){
            vis.insert(query.getInt(0) );
        }
        return vis;
    } catch (SqliteException& e){
        return IdSet();
    }
}

bool SqlitePersistence::saveOrderQueue(const boost::shared_ptr<OrderQueue> oq){
    try{
        std::ostringstream querybuilder;
        querybuilder << "INSERT INTO orderqueue VALUEs (" << oq->getQueueId() << ", " << oq->getObjectId() << ", ";
        querybuilder << (oq->isActive() ? 1 : 0) << ", " << (oq->isRepeating() ? 1 : 0) << ", " << oq->getModTime() << ");";
        singleQuery(querybuilder.str() );

        insertList( "orderslot", oq->getQueueId(), oq->getOrderSlots() );
        insertSet ( "orderqueueowner", oq->getQueueId(), oq->getOwner() );
        insertSet ( "orderqueueallowedtype", oq->getQueueId(), oq->getAllowedOrderTypes() );
        return true;
    } catch (SqliteException& e) {
        return false;
    }
}

bool SqlitePersistence::updateOrderQueue(const boost::shared_ptr<OrderQueue> oq){
    try{
        std::ostringstream querybuilder;
        querybuilder << "UPDATE orderqueue set objectid=" << oq->getObjectId() << ", active=" <<(oq->isActive ? 1 : 0);
        querybuilder << ", repeating=" (oq->isRepeating() ? 1 : 0) << ", modtime=" << oq->getModTime() << " WHERE queueid =" << oq->getQueueId() << ";";
        singleQuery( querybuilder.str() );

        querybuilder.str("");
        querybuilder << "DELETE FROM orderslots WHERE queueid=" << oq->getQueueId() << ";";
        singleQuery( querybuilder.str() );

        querybuilder.str("");
        querybuilder << "DELETE FROM orderqueueowner WHERE queueid=" << oq->getQueueId() << ";";
        singleQuery( querybuilder.str() );

        querybuilder.str("");
        querybuilder << "DELETE FROM orderqueueallowedtype WHERE queueid=" << oq->getQueueId() << ";";
        singleQuery( querybuilder.str() );

        insertList( "orderslot", oq->getQueueId(), oq->getOrderSlots() );
        insertSet ( "orderqueueowner", oq->getQueueId(), oq->getOwner() );
        insertSet ( "orderqueueallowedtype", oq->getQueueId(), oq->getAllowedOrderTypes() );

        return true;
    } catch (SqliteException& e){
        return false;
    }
}

OrderQueue::Ptr SqlitePersistence::retrieveOrderQueue(uint32_t oqid){
    try{
        std::ostringstream querybuilder;


        querybuilder << "SELECT * FROM orderqueue WHERE queueid=" << oqid << ";";

        SqliteQuery query(db, querybuilder.str() );
        OrderQueue::Ptr oq( new OrderQueue(oqid, query.getInt(1), 0) );
        oq->setActive(query.getInt(2) == 1);
        oq->setRepeating(query.getInt(3) == 1);
        oq->setModTime(query.getU64(4));


        querybuilder.str("");
        querybuilder << "SELECT orderid FROM orderslot WHERE queueid=" << oqid << " ORDER BY slot;";

        IdList oolist = idListQuery( querybuilder.str() );
        uint32_t max = (*(std::max_element( oolist.begin(), oolist.end() )));
        oq->setOrderSlots(oolist);
        oq->setNextOrderId(max+1);

        querybuilder.str("");
        querybuilder << "SELECT playerid FROM orderqueueowner WHERE queueid=" << oqid << ";";
        oq->setOwners( idSetQuery( querybuilder.str() ) );

        querybuilder.str("");
        querybuilder << "SELECT ordertype FROM orderqueueallowedtype WHERE queueid=" << oqid << ";";
        oq->setAllowedOrderTypes( idSetQuery( querybuilder.str() ) );

        return oq;
    } catch (SqliteException& e) {
        return OrderQueue::Ptr();
    }
}

bool SqlitePersistence::removeOrderQueue(uint32_t oqid){
    try{
        std::ostringstream querybuilder;
        querybuilder << "DELETE FROM orderslot WHERE queueid=" << oqid << ";";
        singleQuery( querybuilder.str() );

        querybuilder.str("");
        querybuilder << "DELETE FROM orderqueueowner WHERE queueid=" << oqid << ";";
        singleQuery( querybuilder.str() );

        return true;
    } catch ( SqliteException& e ) { return false; }
}

IdSet SqlitePersistence::getOrderQueueIds(){
    try {
        return idSetQuery( "SELECT queueid FROM orderqueue;");
    } catch ( SqliteException& e ) {return IdSet(); }
}

uint32_t SqlitePersistence::getMaxOrderQueueId(){
    try {
        return valueQuery( "SELECT MAX(queueid) FROM orderqueue;" );
    } catch ( SqliteException& e) { return 0; }
}

bool SqlitePersistence::saveOrder(uint32_t queueid, uint32_t ordid, Order* ord){
    try {
        std::ostringstream querybuilder;
        querybuilder << "INSERT INTO ordertype VALUES (" << queueid << ", " << ordid << ", " << ord->getType() << ", " << ord->getTurns() << ");";
        singleQuery( querybuilder.str() );

        insertMap( "orderresource", queueid, ordid, ord->getResources() );

        //store parameters
        uint32_t parampos = 0;
        std::list<OrderParameter*> params = ord->getParameters();
        for(std::list<OrderParameter*>::iterator itcurr = params.begin(); itcurr != params.end(); ++itcurr){
            switch((*itcurr)->getType()){
                case opT_Space_Coord_Abs:
                    updateSpaceCoordParam(queueid, ordid, parampos, static_cast<SpaceCoordParam*>(*itcurr));
                    break;
                case opT_Time:
                    updateTimeParameter(queueid, ordid, parampos, static_cast<TimeParameter*>(*itcurr));
                    break;
                case opT_Object_ID:
                    updateObjectOrderParameter(queueid, ordid, parampos, static_cast<ObjectOrderParameter*>(*itcurr));
                    break;
                case opT_List:
                    updateListParameter(queueid, ordid, parampos, static_cast<ListParameter*>(*itcurr));
                    break;
                case opT_String:
                    updateStringParameter(queueid, ordid, parampos, static_cast<StringParameter*>(*itcurr));
                    break;
                default:
                    Logger::getLogger()->error("SqlitePersistence: unknown order parameter type at save");
                    return false;
            }
            parampos++;
        }

        return true;

    } catch (SqliteException& e) { return false; }
}

bool SqlitePersistence::updateOrder(uint32_t queueid, uint32_t ordid, Order* ord){
    try {
        std::ostringstream querybuilder;
        querybuilder << "UPDATE ordertype SET type = " << ord->getType() << ", turns=" << ord->getTurns() << " WHERE queueid=" << queueid << " AND orderid=" << ordid << ";";
        singleQuery( querybuilder.str() );

        querybuilder.str("");
        querybuilder << "DELETE FROM orderresource WHERE queueid=" << queueid << " AND orderid=" << ordid << ";";
        singleQuery( querybuilder.str() );

        insertMap( "orderresource", queueid, ordid, ord->getResources() );

        //update parameters
        uint32_t parampos = 0;
        std::list<OrderParameter*> params = ord->getParameters();
        for(std::list<OrderParameter*>::iterator itcurr = params.begin(); itcurr != params.end(); ++itcurr){
            switch((*itcurr)->getType()){
                case opT_Space_Coord_Abs:
                    updateSpaceCoordParam(queueid, ordid, parampos, static_cast<SpaceCoordParam*>(*itcurr));
                    break;
                case opT_Time:
                    updateTimeParameter(queueid, ordid, parampos, static_cast<TimeParameter*>(*itcurr));
                    break;
                case opT_Object_ID:
                    updateObjectOrderParameter(queueid, ordid, parampos, static_cast<ObjectOrderParameter*>(*itcurr));
                    break;
                case opT_List:
                    updateListParameter(queueid, ordid, parampos, static_cast<ListParameter*>(*itcurr));
                    break;
                case opT_String:
                    updateStringParameter(queueid, ordid, parampos, static_cast<StringParameter*>(*itcurr));
                    break;
                default:
                    Logger::getLogger()->error("SqlitePersistence: unknown order parameter type at update");
                    return false;
            }
            parampos++;
        }
        return true;
    } catch (SqliteException& e ) { return false; }
}

Order* SqlitePersistence::retrieveOrder(uint32_t queueid, uint32_t ordid){
    Order* order = NULL;
    try {
        std::ostringstream querybuilder;
        {
            querybuilder << "SELECT type,turns FROM ordertype WHERE queueid = " << queueid << " AND orderid = " << ordid << ";";
            SqliteQuery query( db, querybuilder.str() );

            order = Game::getGame()->getOrderManager()->createOrder( query.getInt(0) );
            order->setTurns( query.getInt(1) );
            order->setOrderQueueId(queueid);
        }

        //fetch resources
        {
            querybuilder.str("");
            querybuilder << "SELECT resourceid, amount FROM orderresource WHERE queueid=" << queueid << " AND orderid=" << ordid << ";";
            SqliteQuery query( db, querybuilder.str() );
            while( query.nextRow() ){
                order->addResource(query.getInt(0), query.getInt(1));
            }
        }
        //fetch parameters
        uint32_t parampos = 0;
        std::list<OrderParameter*> params = order->getParameters();
        for(std::list<OrderParameter*>::iterator itcurr = params.begin(); itcurr != params.end(); ++itcurr){
            switch((*itcurr)->getType()){
                case opT_Space_Coord_Abs:
                    retrieveSpaceCoordParam(queueid, ordid, parampos, static_cast<SpaceCoordParam*>(*itcurr));
                    break;
                case opT_Time:
                    retrieveTimeParameter(queueid, ordid, parampos, static_cast<TimeParameter*>(*itcurr));
                    break;
                case opT_Object_ID:
                    retrieveObjectOrderParameter(queueid, ordid, parampos, static_cast<ObjectOrderParameter*>(*itcurr));
                    break;
                case opT_List:
                    retrieveListParameter(queueid, ordid, parampos, static_cast<ListParameter*>(*itcurr));
                    break;
                case opT_String:
                    retrieveStringParameter(queueid, ordid, parampos, static_cast<StringParameter*>(*itcurr));
                    break;
                default:
                    Logger::getLogger()->error("SqlitePersistence: unknown order parameter type at retrieve");
                    return false;
            }
            parampos++;
        }

        return order;
    } catch( SqliteException& e ) {
        delete order;
        return false;
    }
}

bool SqlitePersistence::removeOrder(uint32_t queueid, uint32_t ordid){
    try {
        std::ostringstream querybuilder;
        querybuilder << "SELECT type FROM ordertype WHERE queueid=" << queueid << " AND orderid=" << ordid <<";";
        uint32_t ordertype = valueQuery( querybuilder.str() );

        querybuilder.str("");
        querybuilder << "DELETE FROM ordertype WHERE queueid= " << queueid << " AND orderid=" << ordid << ";";
        singleQuery( querybuilder.str() );

        //remove resources
        querybuilder.str("");
        querybuilder << "DELETE FROM orderresource WHERE queueid=" << queueid << " AND orderid=" << ordid << ";";
        singleQuery( querybuilder.str() );

        //remove parameters
        uint32_t parampos = 0;
        Order* order = Game::getGame()->getOrderManager()->createOrder(ordertype);
        std::list<OrderParameter*> params = order->getParameters();
        for(std::list<OrderParameter*>::iterator itcurr = params.begin(); itcurr != params.end(); ++itcurr){
            switch((*itcurr)->getType()){
                case opT_Space_Coord_Abs:
                    removeSpaceCoordParam(queueid, ordid, parampos);
                    break;
                case opT_Time:
                    removeTimeParameter(queueid, ordid, parampos);
                    break;
                case opT_Object_ID:
                    removeObjectOrderParameter(queueid, ordid, parampos);
                    break;
                case opT_List:
                    removeListParameter(queueid, ordid, parampos);
                    break;
                case opT_String:
                    removeStringParameter(queueid, ordid, parampos);
                    break;
                default:
                    Logger::getLogger()->error("SqlitePersistence: unknown order parameter type at remove");
                    return false;
            }
            parampos++;
        }
        delete order;
        return true;
    } catch( SqliteException& e) {
        return false; 
    }
}

bool SqlitePersistence::updateSpaceCoordParam(uint32_t queueid, uint32_t ordid, uint32_t pos, SpaceCoordParam* scp){
    std::ostringstream querybuilder;
    querybuilder << "DELETE FROM orderparamspace WHERE queueid=" << queueid << " AND orderid=" << ordid << " AND position=" << pos << ";";
    singleQuery( querybuilder.str() );

    querybuilder.str("");
    querybuilder << "INSERT INTO orderparamspace VALUES (" << queueid << ", " << ordid << ", " << pos << ", ";
    querybuilder << scp->getPosition().getX() << ", " << scp->getPosition().getY() << ", ";
    querybuilder << scp->getPosition().getZ() <<");";
    singleQuery( querybuilder.str() );
    return true;
}

bool SqlitePersistence::retrieveSpaceCoordParam(uint32_t queueid, uint32_t ordid, uint32_t pos, SpaceCoordParam* scp){
    std::ostringstream querybuilder;
    querybuilder << "SELECT posx,posy,posz FROM orderparamspace WHERE queueid = " << queueid << " AND orderid = " << ordid << " AND position = " << pos << ";";
    SqliteQuery query( db, querybuilder.str() );
    scp->setPosition(Vector3d(atoll(query.get(0).c_str()), atoll(query.get(1).c_str()), atoll(query.get(2).c_str())));
    return true;
}

bool SqlitePersistence::removeSpaceCoordParam(uint32_t queueid, uint32_t ordid, uint32_t pos){
    std::ostringstream querybuilder;
    querybuilder << "DELETE FROM orderparamspace WHERE queueid=" << queueid << " AND orderid=" << ordid << " AND position=" << pos << ";";
    singleQuery( querybuilder.str() );
    return true;
}

bool SqlitePersistence::updateListParameter(uint32_t queueid, uint32_t ordid, uint32_t pos, ListParameter* lp){
    std::ostringstream querybuilder;
    querybuilder << "DELETE FROM orderparamlist WHERE queueid=" << queueid << " AND orderid=" << ordid << " AND position=" << pos << ";";
    singleQuery( querybuilder.str() );

    IdMap list = lp->getList();
    if(!list.empty()){
        querybuilder.str("");
        querybuilder << "INSERT INTO orderparamlist VALUES ";
        for(IdMap::iterator itcurr = list.begin(); itcurr != list.end(); ++itcurr){
            if(itcurr != list.begin())
                querybuilder << ", ";
            querybuilder << "(" << queueid << ", " << ordid << ", " << pos << ", " << (*itcurr).first << ", " << (*itcurr).second << ")";
        }
        querybuilder << ";";
        singleQuery( querybuilder.str() );
    }
    return true;
}

bool SqlitePersistence::retrieveListParameter(uint32_t queueid, uint32_t ordid, uint32_t pos, ListParameter* lp){
    std::ostringstream querybuilder;
    querybuilder << "SELECT listid, amount FROM orderparamlist WHERE queueid=" << queueid << " AND orderid=" << ordid << " AND position = " << pos << ";";
    lp->setList( idMapQuery( querybuilder.str() ) );
    return true;
}

bool SqlitePersistence::removeListParameter(uint32_t queueid, uint32_t ordid, uint32_t pos){
    std::ostringstream querybuilder;
    querybuilder << "DELETE FROM orderparamlist WHERE queueid=" << queueid << " AND orderid=" << ordid << " AND position=" << pos << ";";
    singleQuery( querybuilder.str() );
    return true;
}

bool SqlitePersistence::updateObjectOrderParameter(uint32_t queueid, uint32_t ordid, uint32_t pos, ObjectOrderParameter* ob){
    std::ostringstream querybuilder;
    querybuilder << "DELETE FROM orderparamobject WHERE queueid=" << queueid << " AND orderid=" << ordid << " AND position=" << pos << ";";
    singleQuery( querybuilder.str() );

    querybuilder.str("");
    querybuilder << "INSERT INTO orderparamobject VALUES (" << queueid << ", " << ordid << ", " << pos << ", ";
    querybuilder << ob->getObjectId() << ");";
    singleQuery( querybuilder.str() );
    return true;
}

bool SqlitePersistence::retrieveObjectOrderParameter(uint32_t queueid, uint32_t ordid, uint32_t pos, ObjectOrderParameter* ob) {
    std::ostringstream querybuilder;
    querybuilder << "SELECT objectid FROM orderparamobject WHERE queueid=" << queueid << " AND orderid = " << ordid << " AND position = " << pos << ";";
    ob->setObjectId(valueQuery( querybuilder.str() ));
    return true;
}

bool SqlitePersistence::removeObjectOrderParameter(uint32_t queueid, uint32_t ordid, uint32_t pos){
    std::ostringstream querybuilder;
    querybuilder << "DELETE FROM orderparamobject WHERE queueid=" << queueid << " AND orderid=" << ordid << " AND position=" << pos << ";";
    singleQuery( querybuilder.str() );
    return true;
}

bool SqlitePersistence::updateStringParameter(uint32_t queueid, uint32_t ordid, uint32_t pos, StringParameter* st){
    std::ostringstream querybuilder;
    querybuilder << "DELETE FROM orderparamstring WHERE queueid=" << queueid << " AND orderid=" << ordid << " AND position=" << pos << ";";
    singleQuery( querybuilder.str() );

    querybuilder.str("");
    querybuilder << "INSERT INTO orderparamstring VALUES (" << queueid << ", " << ordid << ", " << pos << ", '";
    querybuilder << addslashes(st->getString()) <<"');";
    singleQuery( querybuilder.str() );
    return true;
}

bool SqlitePersistence::retrieveStringParameter(uint32_t queueid, uint32_t ordid, uint32_t pos, StringParameter* st){
    std::ostringstream querybuilder;
    querybuilder << "SELECT thestring FROM orderparamstring WHERE queueid=" << queueid << " AND orderid = " << ordid << " AND position = " << pos << ";";
    SqliteQuery query( db, querybuilder.str() );
    st->setString( query.get(0));
    return true;
}

bool SqlitePersistence::removeStringParameter(uint32_t queueid, uint32_t ordid, uint32_t pos){
    std::ostringstream querybuilder;
    querybuilder << "DELETE FROM orderparamstring WHERE queueid=" << queueid << " AND orderid=" << ordid << " AND position=" << pos << ";";
    singleQuery( querybuilder.str() );
    return true;
}

bool SqlitePersistence::updateTimeParameter(uint32_t queueid, uint32_t ordid, uint32_t pos, TimeParameter* tp){
    std::ostringstream querybuilder;
    querybuilder << "DELETE FROM orderparamtime WHERE queueid=" << queueid << " AND orderid=" << ordid << " AND position=" << pos << ";";
    singleQuery( querybuilder.str() );

    querybuilder.str("");
    querybuilder << "INSERT INTO orderparamtime VALUES (" << queueid << ", " << ordid << ", " << pos << ", ";
    querybuilder << tp->getTime() <<");";
    singleQuery( querybuilder.str() );
    return true;
}

bool SqlitePersistence::retrieveTimeParameter(uint32_t queueid, uint32_t ordid, uint32_t pos, TimeParameter* tp){
    std::ostringstream querybuilder;
    querybuilder << "SELECT turns FROM orderparamtime WHERE queueid=" << queueid << " AND orderid = " << ordid << " AND position = " << pos << ";";
    tp->setTime( valueQuery( querybuilder.str() ) );
    return true;
}

bool SqlitePersistence::removeTimeParameter(uint32_t queueid, uint32_t ordid, uint32_t pos){
    std::ostringstream querybuilder;
    querybuilder << "DELETE FROM orderparamtime WHERE queueid=" << queueid << " AND orderid=" << ordid << " AND position=" << pos << ";";
    singleQuery( querybuilder.str() );
    return true;
}

bool SqlitePersistence::saveBoard(Board::Ptr board){
    try {
        std::ostringstream querybuilder;
        querybuilder << "INSERT INTO board VALUES (" << board->getId() << ", '" << addslashes(board->getName()) << "', '";
        querybuilder << addslashes(board->getDescription()) << "', " << board->getNumMessages() << ", ";
        querybuilder << board->getModTime() <<");";
        singleQuery( querybuilder.str() );
        return true;
    } catch( SqliteException& ) {
        return false;
    }
}

bool SqlitePersistence::updateBoard(Board::Ptr board){
    try {
        std::ostringstream querybuilder;
        querybuilder << "UPDATE board SET name='" << addslashes(board->getName()) << "', description='" << addslashes(board->getDescription());
        querybuilder << "', nummessages=" << board->getNumMessages() << ", modtime=" << board->getModTime();
        querybuilder << " WHERE boardid=" << board->getId() << ";";
        singleQuery( querybuilder.str() );
        return true;
    } catch( SqliteException& ) {
        return false;
    }
}

Board::Ptr SqlitePersistence::retrieveBoard(uint32_t boardid){
    try {
        std::ostringstream querybuilder;
        querybuilder << "SELECT * FROM board WHERE boardid = " << boardid << ";";
        SqliteQuery query( db, querybuilder.str() );
        Board::Ptr board( new Board( boardid, query.get(1), query.get(2) ) );
        board->setPersistenceData(query.getInt(3),query.getU64(4));
        return board;
    } catch( SqliteException& ) {
        return Board::Ptr();
    }
}

uint32_t SqlitePersistence::getMaxBoardId(){
    try {
        return valueQuery( "SELECT MAX(boardid) FROM board;" );
    } catch( SqliteException& ) {
        return 0;
    }
}

IdSet SqlitePersistence::getBoardIds(){
    try {
        return idSetQuery( "SELECT boardid FROM board;" );
    } catch( SqliteException& ) {
        return IdSet();
    }
}

bool SqlitePersistence::saveMessage( boost::shared_ptr< Message > msg){
    try {
        uint32_t msgid = msg->getId();
        std::ostringstream querybuilder;
        querybuilder << "INSERT INTO message VALUES (" << msgid << ", '" << addslashes(msg->getSubject()) << "', '";
        querybuilder << addslashes(msg->getBody()) << "', " << msg->getTurn() << ");";
        singleQuery( querybuilder.str() );

        Message::References refs = msg->getReferences();
        if(!refs.empty()){
            querybuilder.str("");
            querybuilder << "INSERT INTO messagereference VALUES ";
            for(Message::References::iterator itcurr = refs.begin(); itcurr != refs.end(); ++itcurr){
                if(itcurr != refs.begin())
                    querybuilder << ", ";
                querybuilder << "(" << msgid << ", " << (*itcurr).first << ", " << (*itcurr).second << ")";
            }
            singleQuery( querybuilder.str() );
        }
        return true;
    } catch( SqliteException& ) {
        return false;
    }
}

boost::shared_ptr< Message > SqlitePersistence::retrieveMessage(uint32_t msgid){
    try {
        std::ostringstream querybuilder;
        querybuilder << "SELECT * FROM message WHERE messageid = " << msgid << ";";
        Message::Ptr msg;
        {
            SqliteQuery query( db, querybuilder.str() );
            msg.reset( new Message() );
            msg->setSubject( query.get(1) );
            msg->setBody( query.get(2) );
            msg->setTurn( query.getInt(3) );
        }

        querybuilder.str("");
        querybuilder << "SELECT type,refid FROM messagereference WHERE messageid = " << msgid << ";";
        {
            SqliteQuery query( db, querybuilder.str() );
            while(query.nextRow()){
                msg->addReference((RefSysType)query.getInt(0),query.getInt(1));
            }
        }
        return msg;
    } catch( SqliteException& ) {
        return Message::Ptr();
    }
}

bool SqlitePersistence::removeMessage(uint32_t msgid){
    try {
        std::ostringstream querybuilder;
        querybuilder << "DELETE FROM message WHERE messageid=" << msgid << ";";
        singleQuery( querybuilder.str() );

        querybuilder.str("");
        querybuilder << "DELETE FROM messagereference WHERE messageid=" << msgid << ";";
        singleQuery( querybuilder.str() );
        return true;
    } catch( SqliteException& ) {
        return false;
    }
}

bool SqlitePersistence::saveMessageList(uint32_t bid, IdList list){
    try {
        std::ostringstream querybuilder;
        querybuilder << "DELETE FROM messageslot WHERE boardid=" << bid <<";";
        singleQuery( querybuilder.str() );
        insertList( "messageslot", bid, list );
        return true;
    } catch( SqliteException& ) {
        return false;
    }
}

IdList SqlitePersistence::retrieveMessageList(uint32_t bid){
    try {
        std::ostringstream querybuilder;
        querybuilder << "SELECT messageid FROM messageslot WHERE boardid=" << bid <<" ORDER BY slot;";
        return idListQuery( querybuilder.str() );
    } catch( SqliteException& ) {
        return IdList();
    }
}

uint32_t SqlitePersistence::getMaxMessageId(){
    try {
        return valueQuery( "SELECT MAX(messageid) FROM message;" );
    } catch( SqliteException& ) {
        return 0;
    }
}

bool SqlitePersistence::saveResource(ResourceDescription::Ptr res){
    try {
        std::ostringstream querybuilder;
        querybuilder << "INSERT INTO resourcedesc VALUES (" << res->getResourceType() << ", '" << addslashes(res->getNameSingular()) << "', '";
        querybuilder << addslashes(res->getNamePlural()) << "', '" << addslashes(res->getUnitSingular()) << "', '";
        querybuilder << addslashes(res->getUnitPlural()) << "', '" << addslashes(res->getDescription()) << "', ";
        querybuilder << res->getMass() << ", " << res->getVolume() << ", " << res->getModTime() << ");";
        singleQuery( querybuilder.str() );
        return true;
    } catch( SqliteException& ) {
        return false;
    }
}

ResourceDescription::Ptr SqlitePersistence::retrieveResource(uint32_t restype){
    ResourceDescription::Ptr res;
    try {
        std::ostringstream querybuilder;
        querybuilder << "SELECT * FROM resourcedesc WHERE resourcetype = " << restype << ";";
        SqliteQuery query( db, querybuilder.str() );

        res.reset( new ResourceDescription() );
        res->setResourceType(restype);
        res->setNameSingular(query.get(1));
        res->setNamePlural(query.get(2));
        res->setUnitSingular(query.get(3));
        res->setUnitPlural(query.get(4));
        res->setDescription(query.get(5));
        res->setMass(query.getInt(6));
        res->setVolume(query.getInt(7));
        res->setModTime(query.getU64(8));
        return res;
    } catch( SqliteException& ) {
        return ResourceDescription::Ptr();
    }
}

uint32_t SqlitePersistence::getMaxResourceId(){
    try {
        return valueQuery( "SELECT MAX(resourcetype) FROM resourcedesc;" );
    } catch( SqliteException& ) {
        return 0;
    }
}

IdSet SqlitePersistence::getResourceIds(){
    try {
        return idSetQuery( "SELECT resourcetype FROM resourcedesc;" );
    } catch( SqliteException& ) {
        return IdSet();
    }
}

bool SqlitePersistence::savePlayer(Player::Ptr player){
    try {
        std::ostringstream querybuilder;
        querybuilder << "INSERT INTO player VALUES (" << player->getID() << ", '" << addslashes(player->getName()) << "', '";
        querybuilder << addslashes(player->getPass()) << "', '" << addslashes(player->getEmail()) << "', '";
        querybuilder << addslashes(player->getComment()) << "', " << player->getBoardId() << ", ";
        querybuilder << ((player->isAlive()) ? 1 : 0) << ", " << player->getModTime() << ");";
        singleQuery( querybuilder.str() );

        PlayerView::Ptr playerview = player->getPlayerView();
        insertMap( "playerscore", player->getID(), player->getAllScores() );
        insertSet( "playerdesignusable", player->getID(), playerview->getUsableDesigns() );
        insertSet( "playercomponentusable", player->getID(), playerview->getUsableComponents() );
        insertSet( "playerobjectowned", player->getID(), playerview->getOwnedObjects() );
        return true;
    } catch( SqliteException& ) {
        return false;
    }
}

bool SqlitePersistence::updatePlayer(Player::Ptr player){
    try {
        std::ostringstream querybuilder;
        querybuilder << "UPDATE player SET name='" << addslashes(player->getName()) << "', password='" << addslashes(player->getPass());
        querybuilder << "', email='" << addslashes(player->getEmail()) << "', comment='" << addslashes(player->getComment());
        querybuilder << "', boardid=" << player->getBoardId() << ", alive=" << ((player->isAlive()) ? 1 : 0);
        querybuilder << ", modtime=" << player->getModTime() << " WHERE playerid=" << player->getID() << ";";
        singleQuery( querybuilder.str() );

        querybuilder.str("");
        querybuilder << "DELETE FROM playerscore WHERE playerid=" << player->getID() << ";";
        singleQuery( querybuilder.str() );

        querybuilder.str("");
        querybuilder << "DELETE FROM playerdesignusable WHERE playerid=" << player->getID() << ";";
        singleQuery( querybuilder.str() );

        querybuilder.str("");
        querybuilder << "DELETE FROM playercomponentusable WHERE playerid=" << player->getID() << ";";
        singleQuery( querybuilder.str() );

        querybuilder.str("");
        querybuilder << "DELETE FROM playerobjectowned WHERE playerid=" << player->getID() << ";";
        singleQuery( querybuilder.str() );

        PlayerView::Ptr playerview = player->getPlayerView();
        insertMap( "playerscore", player->getID(), player->getAllScores() );
        insertSet( "playerdesignusable", player->getID(), playerview->getUsableDesigns() );
        insertSet( "playercomponentusable", player->getID(), playerview->getUsableComponents() );
        insertSet( "playerobjectowned", player->getID(), playerview->getOwnedObjects() );
        return true;
    } catch( SqliteException& ) {
        return false;
    }
}

Player::Ptr SqlitePersistence::retrievePlayer(uint32_t playerid){
    Player::Ptr player;
    try {
        std::ostringstream querybuilder;
        querybuilder << "SELECT * FROM player WHERE playerid = " << playerid << ";";
        {
            SqliteQuery query( db, querybuilder.str() );

            player.reset( new Player( playerid, query.get(1), query.get(2)) );
            player->setEmail(query.get(3));
            player->setComment(query.get(4));
            player->setBoardId(query.getInt(5));
            player->setIsAlive(query.getInt(6) == 1);
            player->setModTime(query.getU64(7));
        }
        {
            querybuilder.str("");
            querybuilder << "SELECT * FROM playerscore WHERE playerid = " << playerid << ";";
            SqliteQuery query( db, querybuilder.str() );
            while(query.nextRow()){
                player->setScore( query.getInt(1), query.getInt(2) );
            }
        }
        PlayerView::Ptr playerview = player->getPlayerView();
        querybuilder.str("");
        querybuilder << "SELECT designid FROM playerdesignview WHERE playerid = " << playerid << ";";
        playerview->setVisibleDesigns( idSetQuery( querybuilder.str() ) );

        querybuilder.str("");
        querybuilder << "SELECT designid FROM playerdesignusable WHERE playerid = " << playerid << ";";
        playerview->setUsableDesigns( idSetQuery( querybuilder.str() ) );

        querybuilder.str("");
        querybuilder << "SELECT componentid FROM playercomponentview WHERE playerid = " << playerid << ";";
        playerview->setVisibleComponents( idSetQuery( querybuilder.str() ) );

        querybuilder.str("");
        querybuilder << "SELECT componentid FROM playercomponentusable WHERE playerid = " << playerid << ";";
        playerview->setUsableComponents( idSetQuery( querybuilder.str() ) );

        querybuilder.str("");
        querybuilder << "SELECT DISTINCT objectid FROM playerobjectview WHERE playerid = " << playerid << ";";
        playerview->setVisibleObjects( idSetQuery( querybuilder.str() ) );

        querybuilder.str("");
        querybuilder << "SELECT objectid FROM playerobjectowned WHERE playerid = " << playerid << ";";
        playerview->setOwnedObjects( idSetQuery( querybuilder.str() ) );

        return player;
    } catch( SqliteException& ) {
        return Player::Ptr();
    }
}

uint32_t SqlitePersistence::getMaxPlayerId(){
    try {
        return valueQuery( "SELECT MAX(playerid) FROM player;" );
    } catch( SqliteException& ) {
        return 0;
    }
}

IdSet SqlitePersistence::getPlayerIds(){
    try {
        return idSetQuery( "SELECT playerid FROM player;" );
    } catch( SqliteException& ) {
        return IdSet();
    }
}

bool SqlitePersistence::saveCategory(Category::Ptr cat){
    try {
        std::ostringstream querybuilder;
        querybuilder << "INSERT INTO category VALUES (" << cat->getCategoryId() << ", '" << cat->getName() << "', '";
        querybuilder << cat->getDescription() << "', " << cat->getModTime() << ");";
        singleQuery( querybuilder.str() );
        return true;
    } catch( SqliteException& ) {
        return false;
    }
}

Category::Ptr SqlitePersistence::retrieveCategory(uint32_t catid){
    try {
        std::ostringstream querybuilder;
        querybuilder << "SELECT * FROM category WHERE categoryid = " << catid << ";";
        SqliteQuery query( db, querybuilder.str() );
        Category::Ptr cat( new Category() );
        cat->setCategoryId(catid);
        cat->setName(query.get(1));
        cat->setDescription(query.get(2));
        cat->setModTime(query.getU64(3));
        return cat;
    } catch( SqliteException& ) {
        return Category::Ptr();
    }
}

uint32_t SqlitePersistence::getMaxCategoryId(){
    try {
        return valueQuery( "SELECT MAX(categoryid) FROM category;" );
    } catch( SqliteException& ) {
        return 0;
    }
}

IdSet SqlitePersistence::getCategoryIds(){
    try {
        return idSetQuery( "SELECT categoryid FROM category;" );
    } catch( SqliteException& ) {
        return IdSet();
    }
}

bool SqlitePersistence::saveDesign(Design::Ptr design){
    try {
        std::ostringstream querybuilder;
        querybuilder << "INSERT INTO design VALUES (" << design->getDesignId() << ", " << design->getCategoryId() << ", '";
        querybuilder << addslashes(design->getName()) << "', '" << addslashes(design->getDescription()) << "', " << design->getOwner() << ", ";
        querybuilder << design->getInUse() << ", " << design->getNumExist() << ", " << design->isValid() << ", '";
        querybuilder << addslashes(design->getFeedback()) << "', " << design->getModTime() << ");";
        singleQuery( querybuilder.str() );

        insertMap( "designcomponent", design->getDesignId(), design->getComponents() );

        PropertyValue::Map proplist = design->getPropertyValues();
        if(!proplist.empty()){
            querybuilder.str("");
            querybuilder << "INSERT INTO designproperty VALUES ";
            for( PropertyValue::Map::iterator itcurr = proplist.begin(); itcurr != proplist.end(); ++itcurr){
                if(itcurr != proplist.begin())
                    querybuilder << ", ";
                PropertyValue pv = itcurr->second;
                querybuilder << "(" << design->getDesignId() << ", " << itcurr->first << ", " << pv.getValue() << ", '";
                querybuilder << addslashes(pv.getDisplayString()) << "')";
            }
            querybuilder << ";";
            singleQuery( querybuilder.str() );
        }
        return true;
    } catch( SqliteException& ) {
        return false;
    }
}

bool SqlitePersistence::updateDesign(Design::Ptr design){
    try {
        std::ostringstream querybuilder;
        querybuilder << "UPDATE design SET categoryid=" << design->getCategoryId() << ", name='";
        querybuilder << addslashes(design->getName()) << "', description='" << addslashes(design->getDescription()) << "', owner=";
        querybuilder << design->getOwner() << ", inuse=" << design->getInUse() << ", numexist=" << design->getNumExist() << ", valid=";
        querybuilder << design->isValid() << ", feedback='" << addslashes(design->getFeedback());
        querybuilder << "', modtime=" << design->getModTime() << " WHERE designid=" << design->getDesignId() << ";";
        singleQuery( querybuilder.str() );

        querybuilder.str("");
        querybuilder << "DELETE FROM designcomponent WHERE designid=" << design->getDesignId() << ";";
        singleQuery( querybuilder.str() );

        querybuilder.str("");
        querybuilder << "DELETE FROM designproperty WHERE designid=" << design->getDesignId() << ";";
        singleQuery( querybuilder.str() );

        insertMap( "designcomponent", design->getDesignId(), design->getComponents() );

        PropertyValue::Map proplist = design->getPropertyValues();
        if(!proplist.empty()){
            querybuilder.str("");
            querybuilder << "INSERT INTO designproperty VALUES ";
            for(PropertyValue::Map::iterator itcurr = proplist.begin(); itcurr != proplist.end(); ++itcurr){
                if(itcurr != proplist.begin())
                    querybuilder << ", ";
                PropertyValue pv = itcurr->second;
                querybuilder << "(" << design->getDesignId() << ", " << itcurr->first << ", " << pv.getValue() << ", '";
                querybuilder << addslashes(pv.getDisplayString()) << "')";
            }
            querybuilder << ";";
            singleQuery( querybuilder.str() );
        }
        return true;
    } catch( SqliteException& ) {
        return false;
    }
}

Design::Ptr SqlitePersistence::retrieveDesign(uint32_t designid){
    Design::Ptr design;
    try {
        std::ostringstream querybuilder;
        {
            querybuilder << "SELECT * FROM design WHERE designid = " << designid << ";";
            SqliteQuery query( db, querybuilder.str() );

            design.reset( new Design() );
            design->setDesignId(designid);
            design->setCategoryId(query.getInt(1));
            design->setName(query.get(2));
            design->setDescription(query.get(3));
            design->setOwner(query.getInt(4));
            design->setInUse(query.getInt(5));
            design->setNumExist(query.getInt(6));
            design->setValid(query.getInt(7),query.get(8));
            design->setModTime(query.getU64(9));
        }

        querybuilder.str("");
        querybuilder << "SELECT componentid,count FROM designcomponent WHERE designid = " << designid << ";";
            design->setComponents(idMapQuery(querybuilder.str()));

        {
            querybuilder.str("");
            querybuilder << "SELECT propertyid,value,displaystring FROM designproperty WHERE designid = " << designid << ";";
            SqliteQuery query( db, querybuilder.str() );
            PropertyValue::Map pvlist;
            while(query.nextRow()){
                PropertyValue pv( atoi(query.get(0).c_str()), atof(query.get(1).c_str()));
                pv.setDisplayString(query.get(2));
                pvlist[pv.getPropertyId()] = pv;
            }
            design->setPropertyValues(pvlist);
        }

        return design;
    } catch( SqliteException& ) {
        return Design::Ptr();
    }
}

uint32_t SqlitePersistence::getMaxDesignId(){
    try {
        return valueQuery( "SELECT MAX(designid) FROM design;" );
    } catch( SqliteException& ) {
        return 0;
    }
}

IdSet SqlitePersistence::getDesignIds(){
    try {
        return idSetQuery( "SELECT designid FROM design;" );
    } catch( SqliteException& ) {
        return IdSet();
    }
}

bool SqlitePersistence::saveComponent(Component::Ptr comp){
    try {
        std::ostringstream querybuilder;
        querybuilder << "INSERT INTO component VALUES (" << comp->getComponentId();
        querybuilder<< ", '" << addslashes(comp->getName()) << "', '" << addslashes(comp->getDescription()) << "', '";
        querybuilder << addslashes(comp->getTpclRequirementsFunction()) << "', " << comp->getModTime() << ");";
        singleQuery( querybuilder.str() );

        insertSet( "componentcat", comp->getId(), comp->getCategoryIds() );

        std::map<uint32_t, std::string> proplist = comp->getPropertyList();
        if(!proplist.empty()){
            querybuilder.str("");
            querybuilder << "INSERT INTO componentproperty VALUES ";
            for(std::map<uint32_t, std::string>::iterator itcurr = proplist.begin(); itcurr != proplist.end(); ++itcurr){
                if(itcurr != proplist.begin())
                    querybuilder << ", ";
                querybuilder << "(" << comp->getComponentId() << ", " << itcurr->first << ", '" << addslashes(itcurr->second) << "')";
            }
            querybuilder << ";";
            singleQuery( querybuilder.str() );
        }
        return true;
    } catch( SqliteException& ) {
        return false;
    }
}

Component::Ptr SqlitePersistence::retrieveComponent(uint32_t compid){
    Component::Ptr comp;
    try {
        std::ostringstream querybuilder;
        {
            querybuilder << "SELECT * FROM component WHERE componentid = " << compid << ";";
            SqliteQuery query( db, querybuilder.str() );

            comp.reset( new Component() );
            comp->setComponentId(compid);
            comp->setName(query.get(1));
            comp->setDescription(query.get(2));
            comp->setTpclRequirementsFunction(query.get(3));
            comp->setModTime(query.getU64(4));
        }

        querybuilder.str("");
        querybuilder << "SELECT categoryid FROM componentcat WHERE componentid = " << compid << ";";
        comp->setCategoryIds(idSetQuery( querybuilder.str() ) );

        {
            querybuilder.str("");
            querybuilder << "SELECT propertyid,tpclvaluefunc FROM componentproperty WHERE componentid = " << compid << ";";
            SqliteQuery query( db, querybuilder.str() );

            std::map<uint32_t, std::string> pvlist;
            while(query.nextRow()) {
                pvlist[query.getInt(0)] = query.get(1);
            }
            comp->setPropertyList(pvlist);
        }
        return comp;
    } catch( SqliteException& ) {
        return Component::Ptr();
    }
}

uint32_t SqlitePersistence::getMaxComponentId(){
    try {
        return valueQuery( "SELECT MAX(componentid) FROM component;" );
    } catch( SqliteException& ) {
        return 0;
    }
}


IdSet SqlitePersistence::getComponentIds(){
    try {
        return idSetQuery( "SELECT componentid FROM component;" );
    } catch( SqliteException& ) {
        return IdSet();
    }
}

bool SqlitePersistence::saveProperty(Property::Ptr prop){
    try {
        std::ostringstream querybuilder;
        querybuilder << "INSERT INTO property VALUES (" << prop->getPropertyId() << ", ";
        querybuilder << prop->getRank() << ", '" << addslashes(prop->getName()) << "', '" << addslashes(prop->getDisplayName());
        querybuilder << "', '" << addslashes(prop->getDescription()) << "', '" << addslashes(prop->getTpclDisplayFunction()) << "', '";
        querybuilder << addslashes(prop->getTpclRequirementsFunction()) << "', " << prop->getModTime() << ");";
        singleQuery( querybuilder.str() );
        insertSet( "propertycat", prop->getPropertyId(), prop->getCategoryIds() );
        return true;
    } catch( SqliteException& ) {
        return false;
    }
}

Property::Ptr SqlitePersistence::retrieveProperty(uint32_t propid){
    Property::Ptr prop;
    try {
        std::ostringstream querybuilder;
        {
            querybuilder << "SELECT * FROM property WHERE propertyid = " << propid << ";";
            SqliteQuery query( db, querybuilder.str() );
            prop.reset( new Property() );
            prop->setPropertyId(propid);
            prop->setRank(query.getInt(1));
            prop->setName(query.get(2));
            prop->setDisplayName(query.get(3));
            prop->setDescription(query.get(4));
            prop->setTpclDisplayFunction(query.get(5));
            prop->setTpclRequirementsFunction(query.get(6));
            prop->setModTime(query.getU64(7));
        }

        querybuilder.str("");
        querybuilder << "SELECT categoryid FROM propertycat WHERE propertyid = " << propid << ";";
        prop->setCategoryIds(idSetQuery( querybuilder.str() ) );
        return prop;
    } catch( SqliteException& ) {
        return Property::Ptr();
    }
}

uint32_t SqlitePersistence::getMaxPropertyId(){
    try {
        return valueQuery( "SELECT MAX(propertyid) FROM property;" );
    } catch( SqliteException& ) {
        return 0;
    }
}

IdSet SqlitePersistence::getPropertyIds(){
    try {
        return idSetQuery( "SELECT propertyid FROM property;" );
    } catch( SqliteException& ) {
        return IdSet();
    }
}

bool SqlitePersistence::saveObjectView(uint32_t playerid, ObjectView::Ptr ov){
    try {
        std::ostringstream querybuilder;
        uint32_t turnnum =  Game::getGame()->getTurnNumber();
        querybuilder << "DELETE FROM playerobjectview WHERE playerid=" << playerid << " AND objectid=" << ov->getObjectId() << " AND turnnum = " << turnnum << ";";
        singleQuery( querybuilder.str() );

        querybuilder.str("");
        querybuilder << "INSERT INTO playerobjectview VALUES (" << playerid << ", " << ov->getObjectId() << ", ";
        querybuilder << turnnum << ", " << ((ov->isCompletelyVisible()) ? 1 : 0) << ", ";
        querybuilder << ((ov->isGone()) ? 1 : 0) << ", " << ((ov->canSeeName()) ? 1 : 0) << ", '";
        querybuilder << addslashes(ov->getVisibleName()) << "', " << ((ov->canSeeDescription()) ? 1 : 0) << ", '";
        querybuilder << addslashes(ov->getVisibleDescription()) << "', " << ov->getModTime() << ");";
        singleQuery( querybuilder.str() );

        //params?

        return true;
    } catch( SqliteException& ) {
        return false;
    }
}

ObjectView::Ptr SqlitePersistence::retrieveObjectView(uint32_t playerid, uint32_t objectid, uint32_t turn){
    try {
        std::ostringstream querybuilder;
        querybuilder << "SELECT * FROM playerobjectview WHERE playerid = " << playerid << " AND objectid = " << objectid << " AND turnnum <= " << turn << " ORDER BY turnnum DESC LIMIT 1;";
        SqliteQuery query( db, querybuilder.str() );
        ObjectView::Ptr obj( new ObjectView() );
        obj->setObjectId(objectid);
        obj->setCompletelyVisible(query.getInt(3) == 1);
        obj->setGone(query.getInt(4) == 1);
        obj->setCanSeeName(query.getInt(5) == 1);
        obj->setVisibleName(query.get(6));
        obj->setCanSeeDescription(query.getInt(7) == 1);
        obj->setVisibleDescription(query.get(8));
        obj->setModTime(query.getU64(9));
        return obj;
    } catch( SqliteException& ) {
        return ObjectView::Ptr();
    }
}

bool SqlitePersistence::saveDesignView(uint32_t playerid, DesignView::Ptr dv){
    try {
        std::ostringstream querybuilder;
        querybuilder << "DELETE FROM playerdesignview WHERE playerid=" << playerid << " AND designid=" << dv->getDesignId() << ";";
        singleQuery( querybuilder.str() );

        querybuilder.str("");
        querybuilder << "DELETE FROM playerdesignviewcomp WHERE playerid=" << playerid << " AND designid=" << dv->getDesignId() << ";";
        singleQuery( querybuilder.str() );

        querybuilder.str("");
        querybuilder << "DELETE FROM playerdesignviewprop WHERE playerid=" << playerid << " AND designid=" << dv->getDesignId() << ";";
        singleQuery( querybuilder.str() );

        querybuilder.str("");
        querybuilder << "INSERT INTO playerdesignview VALUES (" << playerid << ", " << dv->getDesignId() << ", ";
        querybuilder << ((dv->isCompletelyVisible()) ? 1 : 0) << ", " << ((dv->canSeeName()) ? 1 : 0) << ", '";
        querybuilder << addslashes(dv->getVisibleName()) << "', " << ((dv->canSeeDescription()) ? 1 : 0) << ", '";
        querybuilder << addslashes(dv->getVisibleDescription()) << "', " << ((dv->canSeeNumExist()) ? 1 : 0) << ", ";
        querybuilder << dv->getVisibleNumExist() << ", " << ((dv->canSeeOwner()) ? 1 : 0) << ", ";
        querybuilder << dv->getVisibleOwner() << ", " << dv->getModTime() << ");";
        singleQuery( querybuilder.str() );

        insertMap( "playerdesignviewcomp", playerid, dv->getDesignId(), dv->getVisibleComponents() );

        PropertyValue::Map proplist = dv->getVisiblePropertyValues();
        if(!proplist.empty()){
            querybuilder.str("");
            querybuilder << "INSERT INTO playerdesignviewprop VALUES ";
            for(std::map<uint32_t, PropertyValue>::iterator itcurr = proplist.begin(); itcurr != proplist.end();
                    ++itcurr){
                if(itcurr != proplist.begin())
                    querybuilder << ", ";
                querybuilder << "(" << playerid << ", " << dv->getDesignId() << ", " << (itcurr->second.getPropertyId());
                querybuilder << ", '" << addslashes(itcurr->second.getDisplayString()) << "')";
            }
            querybuilder << ";";
            singleQuery( querybuilder.str() );
        }
        return true;
    } catch( SqliteException& ) {
        return false;
    }
}

DesignView::Ptr SqlitePersistence::retrieveDesignView(uint32_t playerid, uint32_t designid){
    try {
        DesignView::Ptr design( new DesignView() );
        std::ostringstream querybuilder;

            querybuilder << "SELECT * FROM playerdesignview WHERE playerid = " << playerid << " AND designid = " << designid << ";";
            SqliteQuery query( db, querybuilder.str() );
            design->setDesignId(designid);
            design->setCompletelyVisible(query.getInt(2) == 1);
            design->setCanSeeName(query.getInt(3) == 1);
            design->setVisibleName(query.get(4));
            design->setCanSeeDescription(query.getInt(5) == 1);
            design->setVisibleDescription(query.get(6));
            design->setCanSeeNumExist(query.getInt(7) == 1);
            design->setVisibleNumExist(query.getInt(8));
            design->setCanSeeOwner(query.getInt(9) == 1);
            design->setVisibleOwner(query.getInt(10));
            design->setModTime(query.getU64(11));


        querybuilder.str("");
        querybuilder << "SELECT componentid,quantity FROM playerdesignviewcomp WHERE playerid = " << playerid << " AND designid = " << designid << ";";
        design->setVisibleComponents(idMapQuery( querybuilder.str() ) );

        querybuilder.str("");
        querybuilder << "SELECT propertyid,value FROM playerdesignviewprop WHERE playerid = " << playerid << " AND designid = " << designid << ";";
        SqliteQuery propquery(db, querybuilder.str() );

        PropertyValue::Map pvlist;
        while(propquery.nextRow()){
            PropertyValue pv( propquery.getInt(0), 0.0);
            pv.setDisplayString(propquery.get(1));
            pvlist[pv.getPropertyId()] = pv;
        }
        design->setVisiblePropertyValues(pvlist);
        return design;
    } catch( SqliteException& ) {
        return DesignView::Ptr();
    }
}

bool SqlitePersistence::saveComponentView(uint32_t playerid, ComponentView::Ptr cv){
    try {
        std::ostringstream querybuilder;
        querybuilder << "DELETE FROM playercomponentview WHERE playerid=" << playerid << " AND componentid=" << cv->getComponentId() << ";";
        singleQuery( querybuilder.str() );

        querybuilder.str("");
        querybuilder << "DELETE FROM playercomponentviewcat WHERE playerid=" << playerid << " AND componentid=" << cv->getComponentId() << ";";
        singleQuery( querybuilder.str() );

        querybuilder.str("");
        querybuilder << "DELETE FROM playercomponentviewproperty WHERE playerid=" << playerid << " AND componentid=" << cv->getComponentId() << ";";
        singleQuery( querybuilder.str() );

        querybuilder.str("");
        querybuilder << "INSERT INTO playercomponentview VALUES (" << playerid << ", " << cv->getComponentId() << ", ";
        querybuilder << ((cv->isCompletelyVisible()) ? 1 : 0) << ", " << ((cv->canSeeName()) ? 1 : 0) << ", '";
        querybuilder << addslashes(cv->getVisibleName()) << "', " << ((cv->canSeeDescription()) ? 1 : 0) << ", '";
        querybuilder << addslashes(cv->getVisibleDescription()) << "', " << ((cv->canSeeRequirementsFunc()) ? 1 : 0) << ", ";
        querybuilder << cv->getModTime() << ");";
        singleQuery( querybuilder.str() );

        insertSet( "playercomponentviewcat", playerid, cv->getComponentId(), cv->getVisibleCategories() );
        insertSet( "playercomponentviewproperty", playerid, cv->getComponentId(), cv->getVisiblePropertyFuncs() );

        return true;
    } catch( SqliteException& ) {
        return false;
    }
}

ComponentView::Ptr SqlitePersistence::retrieveComponentView(uint32_t playerid, uint32_t componentid){
    try {
        ComponentView::Ptr comp;
        std::ostringstream querybuilder;
        {
            querybuilder << "SELECT * FROM playercomponentview WHERE playerid = " << playerid << " AND componentid = " << componentid << ";";
            SqliteQuery query( db, querybuilder.str() );
            comp.reset( new ComponentView( componentid, query.getInt(2) == 1) );
            comp->setCanSeeName(query.getInt(3) == 1);
            comp->setVisibleName(query.get(4));
            comp->setCanSeeDescription(query.getInt(5) == 1);
            comp->setVisibleDescription(query.get(6));
            comp->setCanSeeRequirementsFunc(query.getInt(7) == 1);
            comp->setModTime(query.getU64(8));
        }

        querybuilder.str("");
        querybuilder << "SELECT categoryid FROM playercomponentviewcat WHERE playerid = " << playerid << " AND componentid = " << componentid << ";";
        comp->setVisibleCategories(idSetQuery( querybuilder.str() ) );

        querybuilder.str("");
        querybuilder << "SELECT propertyid FROM playercomponentviewproperty WHERE playerid = " << playerid << " AND componentid = " << componentid << ";";
        comp->setVisiblePropertyFuncs(idSetQuery( querybuilder.str() ));
        return comp;
    } catch( SqliteException& ) {
        return ComponentView::Ptr();
    }
}

// CHECK: importance of adding slashes to the name if the string is passed within quotes
std::string SqlitePersistence::addslashes(const std::string& in) const{
//    char* buf = new char[in.length() * 2 + 1];
//    uint len = mysql_real_escape_string(conn, buf, in.c_str(), in.length());
//    std::string rtv(buf, len);
//    delete[] buf;
    return in;
}

uint32_t SqlitePersistence::getTableVersion(const std::string& name){
    try {
        return valueQuery( "SELECT version FROM tableversion WHERE name='" + addslashes(name) + "';");
    } catch( SqliteException& e) {
        throw std::exception();
    }
}

bool SqlitePersistence::updatePosition3dObjectParam(uint32_t objid, uint32_t turn, uint32_t plid, uint32_t pgroup, uint32_t pgpos, Position3dObjectParam* pob){
    std::ostringstream querybuilder;
    querybuilder << "DELETE FROM objectparamposition WHERE objectid = " << objid << " AND turn = " << turn << " AND playerid = " << plid << " AND paramgroupid = " << pgroup << " AND paramgrouppos = " << pgpos << ";";
    singleQuery( querybuilder.str() );
    querybuilder.str("");
    querybuilder << "INSERT INTO objectparamposition VALUES(" << objid << ", " << turn << ", " << plid << ", " << pgroup << ", " << pgpos << ", " << pob->getPosition().getX() << ", " << pob->getPosition().getY() << ", " << pob->getPosition().getZ() << ", " << pob->getRelative() << ");";
    singleQuery( querybuilder.str() );
    return true;
}

bool SqlitePersistence::retrievePosition3dObjectParam(uint32_t objid, uint32_t turn, uint32_t plid, uint32_t pgroup, uint32_t pgpos, Position3dObjectParam* pob){
    std::ostringstream querybuilder;
    querybuilder << "SELECT posx,posy,posz,relative FROM objectparamposition WHERE objectid = " << objid << " AND turn <= " << turn << " AND playerid = " << plid << " AND paramgroupid = " << pgroup << " AND paramgrouppos = " << pgpos << " ORDER BY turn DESC LIMIT 1;";
    SqliteQuery query( db, querybuilder.str() );
    pob->setPosition(Vector3d(strtoll(query.get(0).c_str(), NULL, 10), strtoll(query.get(1).c_str(), NULL, 10), strtoll(query.get(2).c_str(), NULL, 10)));
    pob->setRelative(query.getInt(3));
    return true;
}

bool SqlitePersistence::updateVelocity3dObjectParam(uint32_t objid, uint32_t turn, uint32_t plid, uint32_t pgroup, uint32_t pgpos, Velocity3dObjectParam* vob){
    std::ostringstream querybuilder;
    querybuilder << "DELETE FROM objectparamvelocity WHERE objectid = " << objid << " AND turn = " << turn << " AND playerid = " << plid << " AND paramgroupid = " << pgroup << " AND paramgrouppos = " << pgpos << ";";
    singleQuery( querybuilder.str() );
    querybuilder.str("");
    querybuilder << "INSERT INTO objectparamvelocity VALUES(" << objid << ", " << turn << ", " << plid << ", " << pgroup << ", " << pgpos << ", " << vob->getVelocity().getX() << ", " << vob->getVelocity().getY() << ", " << vob->getVelocity().getZ() << ", " << vob->getRelative() << ");";
    singleQuery( querybuilder.str() );
    return true;
}

bool SqlitePersistence::retrieveVelocity3dObjectParam(uint32_t objid, uint32_t turn, uint32_t plid, uint32_t pgroup, uint32_t pgpos, Velocity3dObjectParam* vob){
    std::ostringstream querybuilder;
    querybuilder << "SELECT velx,vely,velz,relative FROM objectparamvelocity WHERE objectid = " << objid << " AND turn <= " << turn << " AND playerid = " << plid << " AND paramgroupid = " << pgroup << " AND paramgrouppos = " << pgpos << " ORDER BY turn DESC LIMIT 1;";
    SqliteQuery query( db, querybuilder.str() );
    vob->setVelocity(Vector3d(strtoll(query.get(0).c_str(), NULL, 10), strtoll(query.get(1).c_str(), NULL, 10), strtoll(query.get(2).c_str(), NULL, 10)));
    vob->setRelative(query.getInt(3));
    return true;
}

bool SqlitePersistence::updateOrderQueueObjectParam(uint32_t objid, uint32_t turn, uint32_t plid, uint32_t pgroup, uint32_t pgpos, OrderQueueObjectParam* oob){
    std::ostringstream querybuilder;
    querybuilder << "DELETE FROM objectparamorderqueue WHERE objectid = " << objid << " AND turn = " << turn << " AND playerid = " << plid << " AND paramgroupid = " << pgroup << " AND paramgrouppos = " << pgpos << ";";
    singleQuery( querybuilder.str() );
    querybuilder.str("");
    querybuilder << "INSERT INTO objectparamorderqueue VALUES(" << objid << ", " << turn << ", " << plid << ", " << pgroup << ", " << pgpos << ", " << oob->getQueueId() << ");";
    singleQuery( querybuilder.str() );
    return true;
}

bool SqlitePersistence::retrieveOrderQueueObjectParam(uint32_t objid, uint32_t turn, uint32_t plid, uint32_t pgroup, uint32_t pgpos, OrderQueueObjectParam* oob){
    std::ostringstream querybuilder;
    querybuilder << "SELECT queueid FROM objectparamorderqueue WHERE objectid = " << objid << " AND turn <= " << turn << " AND playerid = " << plid << " AND paramgroupid = " << pgroup << " AND paramgrouppos = " << pgpos << " ORDER BY turn DESC LIMIT 1;";
    oob->setQueueId(valueQuery( querybuilder.str()));
    return true;
}

bool SqlitePersistence::updateResourceListObjectParam(uint32_t objid, uint32_t turn, uint32_t plid, uint32_t pgroup, uint32_t pgpos, ResourceListObjectParam* rob){
    std::ostringstream querybuilder;
    querybuilder << "DELETE FROM objectparamresourcelist WHERE objectid = " << objid << " AND turn = " << turn << " AND playerid = " << plid << " AND paramgroupid = " << pgroup << " AND paramgrouppos = " << pgpos << ";";
    singleQuery( querybuilder.str() );
    querybuilder.str("");
    std::map<uint32_t, std::pair<uint32_t, uint32_t> > reslist = rob->getResources();
    querybuilder << "INSERT INTO objectparamresourcelist VALUES ";
    for(std::map<uint32_t, std::pair<uint32_t, uint32_t> >::iterator itcurr = reslist.begin();
            itcurr != reslist.end(); ++itcurr){
        if(itcurr != reslist.begin()){
            querybuilder << ", ";
        }
        querybuilder << "(" << objid << ", " << turn << ", " << plid << ", " << pgroup << ", " << pgpos << ", " << itcurr->first << ", " << itcurr->second.first << ", " << itcurr->second.second << ")";
    }
    if(reslist.size() == 0){
        //fake resource to make sure there is something, removed when retreived.
        querybuilder << "(" << objid << ", " << turn << ", " << plid << ", " << pgroup << ", " << pgpos << ", 0, 0, 0)";
    }
    querybuilder << ";";
    singleQuery( querybuilder.str() );
    return true;
}

bool SqlitePersistence::retrieveResourceListObjectParam(uint32_t objid, uint32_t turn, uint32_t plid, uint32_t pgroup, uint32_t pgpos, ResourceListObjectParam* rob){
    std::ostringstream querybuilder;
    querybuilder << "SELECT resid, available, possible FROM objectparamresourcelist WHERE objectid = " << objid;
    querybuilder << " AND turn = (SELECT MAX(turn) FROM objectparamresourcelist WHERE objectid = " << objid;
    querybuilder << " AND turn <= " << turn << " AND playerid = " << plid;
    querybuilder << " AND paramgroupid = " << pgroup << " AND paramgrouppos = " << pgpos;
    querybuilder << ") AND playerid = " << plid << " AND paramgroupid = " << pgroup;
    querybuilder << " AND paramgrouppos = " << pgpos << ";";
    SqliteQuery query( db, querybuilder.str() );

    std::map<uint32_t, std::pair<uint32_t, uint32_t> > reslist;
    while(query.nextRow()){
        uint32_t available = query.getInt(1);
        uint32_t possible = query.getInt(2);
        if(available != 0 || possible != 0){
            reslist[query.getInt(0)] = std::pair<uint32_t, uint32_t>(available, possible);
        }
    }
    rob->setResources(reslist);
    return true;
}

bool SqlitePersistence::updateReferenceObjectParam(uint32_t objid, uint32_t turn, uint32_t plid, uint32_t pgroup, uint32_t pgpos, ReferenceObjectParam* rob){
    std::ostringstream querybuilder;
    querybuilder << "DELETE FROM objectparamreference WHERE objectid = " << objid << " AND turn = " << turn << " AND playerid = " << plid << " AND paramgroupid = " << pgroup << " AND paramgrouppos = " << pgpos << ";";
    singleQuery( querybuilder.str() );
    querybuilder.str("");
    querybuilder << "INSERT INTO objectparamreference VALUES(" << objid << ", " << turn << ", " << plid << ", " << pgroup << ", " << pgpos << ", " << rob->getReferenceType() << ", " << rob->getReferencedId() << ");";
    singleQuery( querybuilder.str() );
    return true;
}

bool SqlitePersistence::retrieveReferenceObjectParam(uint32_t objid, uint32_t turn, uint32_t plid, uint32_t pgroup, uint32_t pgpos, ReferenceObjectParam* rob){
    std::ostringstream querybuilder;
    querybuilder << "SELECT reftype, refval FROM objectparamreference WHERE objectid = " << objid << " AND turn <= " << turn << " AND playerid = " << plid << " AND paramgroupid = " << pgroup << " AND paramgrouppos = " << pgpos << " ORDER BY turn DESC LIMIT 1;";
    SqliteQuery query( db, querybuilder.str() );
    rob->setReferenceType(query.getInt(0));
    rob->setReferencedId(query.getInt(1));
    return true;
}

bool SqlitePersistence::updateRefQuantityListObjectParam(uint32_t objid, uint32_t turn, uint32_t plid, uint32_t pgroup, uint32_t pgpos, RefQuantityListObjectParam* rob){
    std::ostringstream querybuilder;
    querybuilder << "DELETE FROM objectparamrefquantitylist WHERE objectid = " << objid << " AND turn = " << turn << " AND playerid = " << plid << " AND paramgroupid = " << pgroup << " AND paramgrouppos = " << pgpos << ";";
    singleQuery( querybuilder.str() );
    querybuilder.str("");
    std::map<std::pair<int32_t, uint32_t>, uint32_t> reflist = rob->getRefQuantityList();
    querybuilder << "INSERT INTO objectparamrefquantitylist VALUES ";
    for(std::map<std::pair<int32_t, uint32_t>, uint32_t>::iterator itcurr = reflist.begin();
            itcurr != reflist.end(); ++itcurr){
        if(itcurr != reflist.begin()){
            querybuilder << ", ";
        }
        querybuilder << "(" << objid << ", " << turn << ", " << plid << ", " << pgroup << ", " << pgpos << ", " << itcurr->first.first << ", " << itcurr->first.second << ", " << itcurr->second << ")";
    }
    if(reflist.size() == 0){
        //fake refquantity to make sure there is something, removed when retreived.
        querybuilder << "(" << objid << ", " << turn << ", " << plid << ", " << pgroup << ", " << pgpos << ", 0, 0, 0)";
    }
    querybuilder << ";";
    singleQuery( querybuilder.str() );
    return true;
}

bool SqlitePersistence::retrieveRefQuantityListObjectParam(uint32_t objid, uint32_t turn, uint32_t plid, uint32_t pgroup, uint32_t pgpos, RefQuantityListObjectParam* rob){
    std::ostringstream querybuilder;
    querybuilder << "SELECT reftype, refid, quant FROM objectparamrefquantitylist WHERE objectid = " << objid << " AND turn = (SELECT MAX(turn) FROM objectparamrefquantitylist WHERE objectid = " << objid;
    querybuilder << " AND turn <= " << turn << " AND playerid = " << plid;
    querybuilder << " AND paramgroupid = " << pgroup << " AND paramgrouppos = " << pgpos;
    querybuilder << ") AND playerid = " << plid << " AND paramgroupid = " << pgroup << " AND paramgrouppos = " << pgpos << ";";
    SqliteQuery query( db, querybuilder.str() );
    std::map<std::pair<int32_t, uint32_t>, uint32_t> reflist;
    while(query.nextRow()){
        int32_t reftype = query.getInt(0);
        uint32_t refid = query.getInt(1);
        uint32_t quant = query.getInt(2);
        if(reftype != 0 && refid != 0 && quant != 0){
            reflist[std::pair<int32_t, uint32_t>(reftype, refid)] = quant;
        }
    }
    rob->setRefQuantityList(reflist);
    return true;
}

bool SqlitePersistence::updateIntegerObjectParam(uint32_t objid, uint32_t turn, uint32_t plid, uint32_t pgroup, uint32_t pgpos, IntegerObjectParam* iob){
    std::ostringstream querybuilder;
    querybuilder << "DELETE FROM objectparaminteger WHERE objectid = " << objid << " AND turn = " << turn << " AND playerid = " << plid << " AND paramgroupid = " << pgroup << " AND paramgrouppos = " << pgpos << ";";
    singleQuery( querybuilder.str() );
    querybuilder.str("");
    querybuilder << "INSERT INTO objectparaminteger VALUES(" << objid << ", " << turn << ", " << plid << ", " << pgroup << ", " << pgpos << ", " << iob->getValue() << ");";
    singleQuery( querybuilder.str() );
    return true;
}

bool SqlitePersistence::retrieveIntegerObjectParam(uint32_t objid, uint32_t turn, uint32_t plid, uint32_t pgroup, uint32_t pgpos, IntegerObjectParam* iob){
    std::ostringstream querybuilder;
    querybuilder << "SELECT val FROM objectparaminteger WHERE objectid = " << objid << " AND turn <= " << turn << " AND playerid = " << plid << " AND paramgroupid = " << pgroup << " AND paramgrouppos = " << pgpos << " ORDER BY turn DESC LIMIT 1;";
    iob->setValue( valueQuery( querybuilder.str()));
    return true;
}

bool SqlitePersistence::updateSizeObjectParam(uint32_t objid, uint32_t turn, uint32_t plid, uint32_t pgroup, uint32_t pgpos, SizeObjectParam* sob){
    std::ostringstream querybuilder;
    querybuilder << "DELETE FROM objectparamsize WHERE objectid = " << objid << " AND turn = " << turn << " AND playerid = " << plid << " AND paramgroupid = " << pgroup << " AND paramgrouppos = " << pgpos << ";";
    singleQuery( querybuilder.str() );
    querybuilder.str("");
    querybuilder << "INSERT INTO objectparamsize VALUES(" << objid << ", " << turn << ", " << plid << ", " << pgroup << ", " << pgpos << ", " << sob->getSize() << ");";
    singleQuery( querybuilder.str() );
    return true;
}

bool SqlitePersistence::retrieveSizeObjectParam(uint32_t objid, uint32_t turn, uint32_t plid, uint32_t pgroup, uint32_t pgpos, SizeObjectParam* sob){
    std::ostringstream querybuilder;
    querybuilder << "SELECT size FROM objectparamsize WHERE objectid = " << objid << " AND turn <= " << turn << " AND playerid = " << plid << " AND paramgroupid = " << pgroup << " AND paramgrouppos = " << pgpos << " ORDER BY turn DESC LIMIT 1;";
    SqliteQuery query( db, querybuilder.str() );
    sob->setSize(query.getU64(0));
    return true;
}

bool SqlitePersistence::updateMediaObjectParam(uint32_t objid, uint32_t turn, uint32_t plid, uint32_t pgroup, uint32_t pgpos, MediaObjectParam* mob){
    std::ostringstream querybuilder;
    querybuilder << "DELETE FROM objectparammedia WHERE objectid = " << objid << " AND turn = " << turn << " AND playerid = " << plid << " AND paramgroupid = " << pgroup << " AND paramgrouppos = " << pgpos << ";";
    singleQuery( querybuilder.str() );
    querybuilder.str("");
    querybuilder << "INSERT INTO objectparammedia VALUES(" << objid << ", " << turn << ", " << plid << ", " << pgroup << ", " << pgpos << ", '" << mob->getMediaUrl() << "');";
    singleQuery( querybuilder.str() );
    return true;
}

bool SqlitePersistence::retrieveMediaObjectParam(uint32_t objid, uint32_t turn, uint32_t plid, uint32_t pgroup, uint32_t pgpos, MediaObjectParam* mob){
    std::ostringstream querybuilder;
    querybuilder << "SELECT url FROM objectparammedia WHERE objectid = " << objid << " AND turn <= " << turn << " AND playerid = " << plid << " AND paramgroupid = " << pgroup << " AND paramgrouppos = " << pgpos << " ORDER BY turn DESC LIMIT 1;";
    SqliteQuery query( db, querybuilder.str() );
    mob->setMediaUrl(query.get(0));
    return true;
}

void SqlitePersistence::lock(){
    SqliteQuery::lock();
}

void SqlitePersistence::unlock(){
    SqliteQuery::unlock();
}

void SqlitePersistence::idSetToStream( std::ostringstream& stream, const uint32_t id, const IdSet& idset ) const {
    for ( IdSet::const_iterator it = idset.begin(); it != idset.end(); ++it ) {
        if ( it != idset.begin() ) stream << ", ";
        stream << "(" << id << ", " << (*it) << ")";
    }
}

void SqlitePersistence::idSetToStream( std::ostringstream& stream, const uint32_t id, const uint32_t id2, const IdSet& idset ) const {
    for ( IdSet::const_iterator it = idset.begin(); it != idset.end(); ++it ) {
        if ( it != idset.begin() ) stream << ", ";
        stream << "(" << id << ", " << id2 << ", " << (*it) << ")";
    }
}

void SqlitePersistence::idListToStream( std::ostringstream& stream, const uint32_t id, const IdList& idlist ) const {
    uint32_t slotnum = 0;
    for(IdList::const_iterator it = idlist.begin(); it != idlist.end(); ++it) {
        if ( it != idlist.begin() ) stream << ", ";
        stream << "(" << id << ", " << slotnum << ", " << (*it) << ")";
        slotnum++;
    }
}

void SqlitePersistence::idMapToStream( std::ostringstream& stream, const uint32_t id, const IdMap& idmap ) const {
    for ( IdMap::const_iterator it = idmap.begin(); it != idmap.end(); ++it ) {
        if ( it != idmap.begin() ) stream << ", ";
        stream << "(" << id << ", " << it->first << ", " << it->second << ")";
    }
}

void SqlitePersistence::idMapToStream( std::ostringstream& stream, const uint32_t id, const uint32_t id2, const IdMap& idmap ) const {
    for ( IdMap::const_iterator it = idmap.begin(); it != idmap.end(); ++it ) {
        if ( it != idmap.begin() ) stream << ", ";
        stream << "(" << id << ", " << id2 << ", " << it->first << ", " << it->second << ")";
    }
}

void  SqlitePersistence::insertSet ( const std::string& table, uint32_t id, const IdSet& idset ) {
    if (idset.empty()) return;
    std::ostringstream query;
    query << "INSERT INTO " << table << " VALUES ";
    idSetToStream( query, id, idset );
    query << ";";
    singleQuery( query.str() );
}

void  SqlitePersistence::insertSet ( const std::string& table, uint32_t id, uint32_t id2, const IdSet& idset ) {
    if (idset.empty()) return;
    std::ostringstream query;
    query << "INSERT INTO " << table << " VALUES ";
    idSetToStream( query, id, id2, idset );
    query << ";";
    singleQuery( query.str() );
}

void  SqlitePersistence::insertList( const std::string& table, uint32_t id, const IdList& idlist ) {
    if (idlist.empty()) return;
    std::ostringstream query;
    query << "INSERT INTO " << table << " VALUES ";
    idListToStream( query, id, idlist );
    query << ";";
    singleQuery( query.str() );
}

void  SqlitePersistence::insertMap ( const std::string& table, uint32_t id, const IdMap& idmap ){
    if (idmap.empty()) return;
    std::ostringstream query;
    query << "INSERT INTO " << table << " VALUES ";
    idMapToStream( query, id, idmap );
    query << ";";
    singleQuery( query.str() );
}

void  SqlitePersistence::insertMap ( const std::string& table, uint32_t id, uint32_t id2, const IdMap& idmap ){
    if (idmap.empty()) return;
    std::ostringstream query;
    query << "INSERT INTO " << table << " VALUES ";
    idMapToStream( query, id, id2, idmap );
    query << ";";
    singleQuery( query.str() );
}

void SqlitePersistence::singleQuery( const std::string& query ) {
    SqliteQuery q( db, query );
}

uint32_t SqlitePersistence::valueQuery( const std::string& query ) {
    SqliteQuery q( db, query );
    if(q.validRow()){
        return q.getInt(0);
    }
    return 0;
}

const IdSet SqlitePersistence::idSetQuery( const std::string& query ) {
    SqliteQuery q( db, query );
    IdSet set;
    while(q.nextRow()){
        set.insert(q.getInt(0));
    }
    return set;
}

const IdList SqlitePersistence::idListQuery( const std::string& query ) {
    SqliteQuery q( db, query );
    IdList list;
    while(q.nextRow()){
        list.push_back(q.getInt(0));
    }
    return list;
}

const IdMap SqlitePersistence::idMapQuery( const std::string& query ) {
    SqliteQuery q( db, query );
    IdMap map;
    while(q.nextRow()){
        map[ q.getInt(0) ] = q.getInt(1);
    }
    return map;
}


SqliteQuery::SqliteQuery( sqlite3* db, const std::string& new_query )
: database( db ), result( -1 ), row( false ), query( new_query ), db_err( NULL )
{
    lock();
    //sqlite3_stmt stmt;
    if ( sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, NULL) != 0 ) {
        unlock(); // Destructor WON'T get called if throw is in constructor
        throw SqliteException(database,  "Query '"+query+"' failed!");
    }

}

const std::string SqliteQuery::get( uint32_t index ) {
        if ( result == -1 )
        {
            fetchResult();
            nextRow();
        }
        if ( row == false ) {
            throw SqliteException( database, "Query '"+query+"' row empty!");
        }
        const char* item = (const char*)(sqlite3_column_text(stmt, index));

        return item;
  }

int SqliteQuery::getInt( uint32_t index ) {
    if ( result == -1 ) {
        fetchResult();
        nextRow();
    }
    if ( row == false ) {
        throw SqliteException( database, "Query '"+query+"' row empty!");
    }
    if ( sqlite3_column_text(stmt, index) == NULL ){
        throw SqliteException( database, "Int value is NULL");
    }
    return sqlite3_column_int(stmt, index);
}

uint64_t SqliteQuery::getU64( uint32_t index ) {
    if ( result == -1 ) {
        fetchResult();
        nextRow();
    }
    if ( row == false ) {
        throw SqliteException( database, "Query '"+query+"' row empty!");
    }
    if ( (sqlite3_column_text(stmt, index)) == NULL ){
        throw SqliteException( database, "UInt64 value is NULL");
    }
    const char* item = (const char*)(sqlite3_column_text(stmt, index));

    return strtoull(item,NULL,10);
}

void SqliteQuery::fetchResult() {
    //result = mysql_store_result(connection);
    //if (result == NULL) //empty result or error
    result = sqlite3_step(stmt);
    if ( result != SQLITE_ROW ) {
        throw SqliteException( database, "Query '"+query+"' result failed!");
    }
    unlock();
}

bool SqliteQuery::validRow() {
    if ( result == -1 ) {
        fetchResult();
        nextRow();
    }
    return (row);
}

bool SqliteQuery::nextRow() {
    if ( result == -1 ) fetchResult();
    if ( row == true ) fetchResult();
    row = (result == SQLITE_ROW);
    return row;
}

SqliteQuery::~SqliteQuery() {
    if ( result != -1 ) sqlite3_finalize(stmt);
    unlock(); // unlock needs to be multiunlock safe
}
