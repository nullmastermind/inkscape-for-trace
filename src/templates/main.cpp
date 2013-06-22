#include <gtkmm/main.h>

#include "new-from-template.h"

using namespace Inkscape::UI;

int main (int argc, char *argv[])
{
  Gtk::Main kit(argc, argv);

  NewFromTemplate dialog;
  dialog.run();
  //Gtk::Main::run(dialog);

  return 0;
}
