#pragma once

#include "factory.h"
#include "singleton.h"

class SPObject;
typedef Singleton< Factory<SPObject> > SPFactory;
