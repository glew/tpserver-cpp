

#include "settings.h"

Settings* Settings::myInstance = NULL;

Settings* Settings::getSettings(){
  if(myInstance == NULL){
    myInstance = new Settings();
  }
  return myInstance;
}

/*
void Settings::set(int item, int value){

}

void Settings::set(int item, String value){

}

void set(int item, bool value){
  
}

int Settings::get(int item){
  return -0;
}

String Settings::get(int item){
  return "";
}

bool Settings::get(int item){
  return false;
}
*/

Settings::Settings(){

}

Settings::~Settings(){

}

Settings::Settings(Settings &rhs){}

Settings Settings::operator=(Settings &rhs){}
