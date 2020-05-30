#pragma once
#include "sudohstdlib.h"

#define True Variable(Bool::t)
#define False Variable(Bool::f)
#define null Variable()

#define map (std::shared_ptr<Variable::Map>)new Variable::Map
#define list (std::shared_ptr<Variable::List>)new Variable::List

typedef Variable var;
