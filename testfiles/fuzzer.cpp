#include "xml/repr.h"
#include "inkscape.h"
#include "document.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    g_type_init();
    Inkscape::GC::init();
    if ( !Inkscape::Application::exists() )
        Inkscape::Application::create("", false);
    //void* a= sp_repr_read_mem((const char*)data, size, 0);
    SPDocument *doc = SPDocument::createNewDocFromMem( (const char*)data, size, 0);
    if(doc)
        doc->doUnref();
    return 0;
}
