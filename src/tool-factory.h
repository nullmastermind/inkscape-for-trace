#pragma once

#include "factory.h"
#include "singleton.h"

class SPEventContext;
typedef Singleton< Factory<SPEventContext> > ToolFactory;
