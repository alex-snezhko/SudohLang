#pragma once
#include "sudohstdlib.h"

#define True Variable(Bool::t)
#define False Variable(Bool::f)
#define null Variable()

#define list (std::shared_ptr<Variable::List>)new Variable::List
#define map (std::shared_ptr<Variable::Map>)new Variable::Map

typedef Variable var;
