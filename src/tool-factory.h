#ifndef TOOL_FACTORY_SEEN
#define TOOL_FACTORY_SEEN

#include "factory.h"

class SPEventContext;
typedef Singleton< Factory<SPEventContext> > ToolFactory;


#endif
