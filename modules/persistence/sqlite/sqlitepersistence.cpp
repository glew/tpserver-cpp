/* 
 * File:   sqlitepersistence.cpp
 * Author: greg
 * 
 * Created on June 17, 2010, 10:54 AM
 */

//included in persistence.h
#include <stdint.h>

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

#include <my_global.h>
#include <my_sys.h>
#include <sqlite3.h>

#include "sqlitepersistence.h"

// TODO:
//extern "C" ...

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
    std::string ssock = conf->get("mysql_socket");
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
                for(ObjectParameterGroup::Mapp::iterator itcurr = groups.begin();
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
        query.nextRob();

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
                        ObjectParameter* parameter *(paramcurr);
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
        querybuidler << (oq->isActive() ? 1 : 0) << ", " << (oq->isRepeating() ? 1 : 0) << ", " << oq->getModTime() << ");";
        singleQuery(querybuilder.str() );

        insertList( "orderslot", oq->getQueueID(), oq->getOrderSlots() );
        insertSet ( "orderqueueowner", oq->getQueueID(), oq->getOwner() );
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
        querybuidler << ", repeating=" (oq->isRepeating() ? 1 : 0) << ", modtime=" << oq->getModTime() << " WHERE queueid =" << oq->getQueueID() << ";";
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
            MysqlQuery query( conn, querybuilder.str() );

            order = Game::getGame()->getOrderManager()->createOrder( query.getInt(0) );
            order->setTurns( query.getInt(1) );
            order->setOrderQueueId(queueid);
        }

        //fetch resources
        {
            querybuilder.str("");
            querybuilder << "SELECT resourceid, amount FROM orderresource WHERE queueid=" << queueid << " AND orderid=" << ordid << ";";
            MysqlQuery query( conn, querybuilder.str() );
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
    } catch( MysqlException& ) {
        return false; 
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
    } catch( SqliteException& ) {
        throw std::exception();
    }
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

SqliteQuery::SqliteQuery( sqlite3* db, const std::string& new_query )
: database( db ), result( NULL ), row( NULL ), query( new_query ), db_err( NULL )
{
    lock();
    //sqlite3_stmt stmt;
    if ( sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, NULL) != 0 ) {
        unlock(); // Destructor WON'T get called if throw is in constructor
        throw SqliteException(database,  "Query '"+query+"' failed!");
    }

}

const std::string SqliteQuery::get( uint32_t index ) {
        if ( result == NULL )
        {
            fetchResult();
            nextRow();
        }
        if ( row == NULL ) {
            throw SqliteException( database, "Query '"+query+"' row empty!");
        }
        return (sqlite3_column_text(stmt, index));
  }

int SqliteQuery::getInt( uint32_t index ) {
    if ( result == NULL ) {
        fetchResult();
        nextRow();
    }
    if ( row == NULL ) {
        throw SqliteException( database, "Query '"+query+"' row empty!");
    }
    if ( row[index] == NULL ){
        throw SqliteException( database, "Int value is NULL");
    }
    return sqlite3_column_int(stmt, index);
}

uint64_t SqliteQuery::getU64( uint32_t index ) {
    if ( result == NULL ) {
        fetchResult();
        nextRow();
    }
    if ( row == NULL ) {
        throw SqliteException( database, "Query '"+query+"' row empty!");
    }
    if ( row[index] == NULL ){
        throw SqliteException( database, "UInt64 value is NULL");
    }
    return strtoull((sqlite3_column_text(stmt, index)),NULL,10);
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
    if ( result == NULL ) {
        fetchResult();
        nextRow();
    }
    return row != NULL;
}

bool SqliteQuery::nextRow() {
    if ( result == NULL ) fetchResult();
    row = (result == SQLITE_ROW);
    return row;
}

SqliteQuery::~SqliteQuery() {
    if ( result != NULL ) sqlite3_finalize(stmt);
    unlock(); // unlock needs to be multiunlock safe
}
