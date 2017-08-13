/**
 * @file
 * Inkscape About box - implementation.
 */
/* Authors:
 *   Derek P. Moore <derekm@hackunix.org>
 *   MenTaLguY <mental@rydia.net>
 *   Kees Cook <kees@outflux.net>
 *   Jon Phillips <jon@rejon.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2004 Derek P. Moore
 * Copyright 2004 Kees Cook
 * Copyright 2004 Jon Phillips
 * Copyright 2005 MenTaLguY
 *
 * Released under GNU GPL.  Read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "ui/dialog/aboutbox.h"

#include <glibmm/fileutils.h>
#include <glibmm/i18n.h>
#include <glibmm/miscutils.h>

#include <gtkmm/aspectframe.h>
#include <gtkmm/textview.h>

#include "path-prefix.h"
#include "document.h"
#include "svg-view-widget.h"
#include "sp-text.h"
#include "text-editing.h"
#include "ui/icon-names.h"
#include "util/units.h"

#include "inkscape-version.h"


namespace Inkscape {
namespace UI {
namespace Dialog {



static Gtk::Widget *build_splash_widget();

static AboutBox *window=NULL;

void AboutBox::show_about() {
    if (!window)
        window = new AboutBox();
    window->show();
}

void AboutBox::hide_about() {
    if (window)
        window->hide();
}

/**
 * Constructor
 */ 
AboutBox::AboutBox() {

    // call this first
    initStrings();

    Gtk::Widget *splash=build_splash_widget();
    if (splash) {
        get_content_area()->pack_end(*manage(splash), true, true);
        splash->show_all();
    }

    set_logo_icon_name(INKSCAPE_ICON("inkscape"));
    set_program_name("Inkscape");
    set_version(Inkscape::version_string);

    set_website("https://www.inkscape.org");

    Gtk::Requisition minimum_size;
    Gtk::Requisition natural_size;
    get_preferred_size(minimum_size, natural_size);
    
    // allow window to shrink
    set_size_request(0, 0);
    set_default_size(minimum_size.width, minimum_size.height);
}

Gtk::Widget *build_splash_widget() {
    /* TRANSLATORS: This is the filename of the `About Inkscape' picture in
       the `screens' directory.  Thus the translation of "about.svg" should be
       the filename of its translated version, e.g. about.zh.svg for Chinese.

       Please don't translate the filename unless the translated picture exists. */

    // Try to get the translated version of the 'About Inkscape' file first.  If the
    // translation fails, or if the file does not exist, then fall-back to the
    // default untranslated "about.svg" file
    //
    // FIXME? INKSCAPE_SCREENSDIR and "about.svg" are in UTF-8, not the
    // native filename encoding... and the filename passed to sp_document_new
    // should be in UTF-*8..
    auto about = Glib::build_filename(INKSCAPE_SCREENSDIR, _("about.svg"));
    if (!Glib::file_test (about, Glib::FILE_TEST_EXISTS)) {
        about = Glib::build_filename(INKSCAPE_SCREENSDIR, "about.svg");
    }

    // Create an Inkscape document from the 'About Inkscape' picture
    SPDocument *doc=SPDocument::createNewDoc (about.c_str(), TRUE);
    g_return_val_if_fail(doc != NULL, NULL);

    SPObject *version = doc->getObjectById("version");
    if ( version && SP_IS_TEXT(version) ) {
        sp_te_set_repr_text_multiline (SP_TEXT (version), Inkscape::version_string);
    }
    doc->ensureUpToDate();

    // TODO: Return a Gdk::Pixbuf instead of a widget, for better integration
    // with the parent Gtk::AboutDialog class
    GtkWidget *v=sp_svg_view_widget_new(doc);

    // temporary hack: halve the dimensions so the dialog will fit
    double width=doc->getWidth().value("px") / 2;
    double height=doc->getHeight().value("px") / 2;
    
    doc->doUnref();

    SP_SVG_VIEW_WIDGET(v)->setResize(false, static_cast<int>(width), static_cast<int>(height));

    Gtk::AspectFrame *frame=new Gtk::AspectFrame();
    frame->unset_label();
    frame->set_shadow_type(Gtk::SHADOW_NONE);
    frame->property_ratio() = width / height;
    frame->add(*manage(Glib::wrap(v)));

    return frame;
}

/**
 * This method must be called before any of the texts are
 * used for making widgets
 */  
void AboutBox::initStrings() {

    //##############################
    //# A U T H O R S
    //##############################
    
    /* This text is copied from the AUTHORS file.
     * To update it, execute this snippet of sed magic in the toplevel
     * source directory:
     *
     * sed -e 's/^\(.*\) \([^ ]*\)*$/\2_ \1/' AUTHORS
           | sort
           | sed -e 's/^\([^_]*\)_ \(.*\)$/\2 \1/;s/^.*$/"\0\\n"/;$s/\\n//'
           | zenity --text-info
     *
     * and paste the result from the combo box here.
     */ 
    std::vector<Glib::ustring> authors = {
        "Maximilian Albert",
        "Joshua A. Andler",
        "Tavmjong Bah",
        "Pierre Barbry-Blot",
        "Jean-François Barraud",
        "Campbell Barton",
        "Bill Baxter",
        "John Beard",
        "John Bintz",
        "Arpad Biro",
        "Nicholas Bishop",
        "Joshua L. Blocher",
        "Hanno Böck",
        "Tomasz Boczkowski",
        "Adrian Boguszewski",
        "Henrik Bohre",
        "Boldewyn",
        "Daniel Borgmann",
        "Bastien Bouclet",
        "Hans Breuer",
        "Gustav Broberg",
        "Christopher Brown",
        "Marcus Brubaker",
        "Luca Bruno",
        "Brynn (brynn@inkscapecommunity.com)",
        "Nicu Buculei",
        "Bulia Byak",
        "Pierre Caclin",
        "Ian Caldwell",
        "Gail Carmichael",
        "Ed Catmur",
        "Chema Celorio",
        "Jabiertxo Arraiza Cenoz",
        "Johan Ceuppens",
        "Zbigniew Chyla",
        "Alexander Clausen",
        "John Cliff",
        "Kees Cook",
        "Ben Cromwell",
        "Robert Crosbie",
        "Jon Cruz",
        "Aurélie De-Cooman",
        "Milosz Derezynski",
        "Daniel Díaz",
        "Bruno Dilly",
        "Larry Doolittle",
        "Nicolas Dufour",
        "Tim Dwyer",
        "Maxim V. Dziumanenko",
        "Moritz Eberl",
        "Johan Engelen",
        "Miklos Erdelyi",
        "Ulf Erikson",
        "Noé Falzon",
        "Sebastian Faubel",
        "Frank Felfe",
        "Andrew Fitzsimon",
        "Edward Flick",
        "Marcin Floryan",
        "Ben Fowler",
        "Fred",
        "Cedric Gemy",
        "Steren Giannini",
        "Olivier Gondouin",
        "Ted Gould",
        "Toine de Greef",
        "Michael Grosberg",
        "Kris De Gussem",
        "Bryce Harrington",
        "Dale Harvey",
        "Aurélio Adnauer Heckert",
        "Carl Hetherington",
        "Jos Hirth",
        "Hannes Hochreiner",
        "Thomas Holder",
        "Joel Holdsworth",
        "Christoffer Holmstedt",
        "Alan Horkan",
        "Karl Ove Hufthammer",
        "Richard Hughes",
        "Nathan Hurst",
        "inductiveload",
        "Thomas Ingham",
        "Jean-Olivier Irisson",
        "Bob Jamison",
        "Ted Janeczko",
        "Marc Jeanmougin",
        "jEsuSdA",
        "Fernando Lucchesi Bastos Jurema",
        "Lauris Kaplinski",
        "Lynn Kerby",
        "Niko Kiirala",
        "James Kilfiger",
        "Nikita Kitaev",
        "Jason Kivlighn",
        "Adrian Knoth",
        "Krzysztof Kosiński",
        "Petr Kovar",
        "Benoît Lavorata",
        "Alex Leone",
        "Julien Leray",
        "Raph Levien",
        "Diederik van Lierop",
        "Nicklas Lindgren",
        "Vitaly Lipatov",
        "Ivan Louette",
        "Pierre-Antoine Marc",
        "Aurel-Aimé Marmion",
        "Colin Marquardt",
        "Craig Marshall",
        "Ivan Masár",
        "Dmitry G. Mastrukov",
        "David Mathog",
        "Matiphas",
        "Michael Meeks",
        "Federico Mena",
        "MenTaLguY",
        "Aubanel Monnier",
        "Vincent Montagne",
        "Tim Mooney",
        "Derek P. Moore",
        "Chris Morgan",
        "Peter Moulder",
        "Jörg Müller",
        "Yukihiro Nakai",
        "Victor Navez",
        "Christian Neumair",
        "Nick",
        "Andreas Nilsson",
        "Mitsuru Oka",
        "Vinícius dos Santos Oliveira",
        "Martin Owens",
        "Alvin Penner",
        "Matthew Petroff",
        "Jon Phillips",
        "Zdenko Podobny",
        "Alexandre Prokoudine",
        "Jean-René Reinhard",
        "Alexey Remizov",
        "Frederic Rodrigo",
        "Hugo Rodrigues",
        "Juarez Rudsatz",
        "Xavier Conde Rueda",
        "Felipe Corrêa da Silva Sanches",
        "Christian Schaller",
        "Marco Scholten",
        "Tom von Schwerdtner",
        "Danilo Šegan",
        "Abhishek Sharma",
        "Shivaken",
        "Michael Sloan",
        "John Smith",
        "Sandra Snan",
        "Boštjan Špetič",
        "Aaron Spike",
        "Kaushik Sridharan",
        "Ralf Stephan",
        "Dariusz Stojek",
        "Martin Sucha",
        "~suv",
        "Pat Suwalski",
        "Adib Taraben",
        "Hugh Tebby",
        "Jonas Termeau",
        "David Turner",
        "Andre Twupack",
        "Aleksandar Urošević",
        "Alex Valavanis",
        "Joakim Verona",
        "Lucas Vieites",
        "Daniel Wagenaar",
        "Liam P. White",
        "Sebastian Wüst",
        "Michael Wybrow",
        "Gellule Xg",
        "Daniel Yacob",
        "Masatake Yamato",
        "David Yip"};
    set_authors(authors);

    //##############################
    //# T R A N S L A T O R S
    //##############################

    Glib::ustring translators_text;

    // TRANSLATORS: Put here your name (and other national contributors')      
    // one per line in the form of: name surname (email). Use \n for newline.
    Glib::ustring thisTranslation = _("translator-credits");

    /**
     * See if the translators for the current language
     * made an entry for "translator-credits".  If it exists,
     * put it at the top of the window,  add some space between
     * it and the list of all translators.
     *      
     * NOTE:  Do we need 2 more .po entries for titles:
     *  "translators for this language"
     *  "all translators"  ??     
     */                          
    if (thisTranslation != "translator-credits") {
        translators_text.append(thisTranslation);
        translators_text.append("\n\n\n");
    }

    /* This text is copied from the TRANSLATORS file.
     * To update it, execute this snippet of sed magic in the toplevel
     * source directory:
     *
     * sed -e 's/^\(.*\) \([^ ]*\)*$/\2_ \1/' TRANSLATORS
           | sed -e 's/^\([^_]*\)_ \(.*\)$/\2 \1/;s/^.*$/"\0\\n"/;$s/\\n//'
           | zenity --text-info
     *
     * and paste the result from the combo box here.
     */          
    gchar const *allTranslators =
"3ARRANO.com <3arrano@3arrano.com>, 2005.\n"
"Adib Taraben <theadib@gmail.com>, 2004-2014.\n"
"Alan Monfort <alan.monfort@free.fr>, 2009-2010.\n"
"Alastair McKinstry <mckinstry@computer.org>, 2000.\n"
"Aleksandar Marković <alleksandar.markovic@gmail.com>, 2015.\n"
"Aleksandar Urošević <urke@users.sourceforge.net>, 2004-2009.\n"
"Alessio Frusciante <algol@firenze.linux.it>, 2002, 2003.\n"
"Alexander Shopov <ash@contact.bg>, 2006.\n"
"Alexandre Prokoudine <alexandre.prokoudine@gmail.com>, 2005, 2010-2014.\n"
"Alexey Remizov <alexey@remizov.pp.ru>, 2004.\n"
"Ali Ghanavatian <ghanvatian.ali@gmail.com>, 2010.\n"
"Álvaro Lopes <alvieboy@alvie.com>, 2001, 2002.\n"
"Andreas Hyden <a.hyden@cyberpoint.se>, 2000.\n"
"Andrius Ramanauskas <knutux@gmail.com>, 2006.\n"
"Antonio Codazzi <f_sophia@libero.it>, 2006, 2007.\n"
"Antônio Cláudio (LedStyle) <ledstyle@gmail.com>, 2006.\n"
"Amanpreet Singh Brar Alamwalia <apbrar@gmail.com>, 2005.\n"
"Arman Aksoy <armish@linux-sevenler.de>, 2003.\n"
"Arpad Biro <biro_arpad@yahoo.com>, 2004, 2005.\n"
"Benedikt Roth <Benedikt.Roth@gmx.net>, 2000.\n"
"Benjamin Weis <benjamin.weis@gmx.com>, 2014.\n"
"Benno Schulenberg <benno@vertaalt.nl>,  2008.\n"
"Boštjan Špetič <igzebedze@cyberpipe.org>, 2004, 2005.\n"
"Brisa Francesco <fbrisa@yahoo.it>, 2000.\n"
"Bruce Cowan <bruce@bcowan.me.uk>, 2010.\n"
"bulia byak <buliabyak@users.sf.net>, 2004.\n"
"Chris jia <Chrisjiasl@gmail.com>, 2006.\n"
"Christian Meyer <chrisime@gnome.org>, 2000-2002.\n"
"Christian Neumair <chris@gnome-de.org>, 2002, 2003.\n"
"Christian Rose <menthos@menthos.com>, 2000-2003.\n"
"Cristian Secară <cristi@secarica.ro>, 2010-2013.\n"
"Christophe Merlet (RedFox) <redfox@redfoxcenter.org>, 2000-2002.\n"
"Clytie Siddall <clytie@riverland.net.au>, 2004-2008.\n"
"Colin Marquardt <colin@marquardt-home.de>, 2004-2006.\n"
"Cédric Gemy <radar.map35@free.fr>, 2006.\n"
"Daniel Díaz <yosoy@danieldiaz.org>, 2004.\n"
"Didier Conchaudron <conchaudron@free.fr>, 2003.\n"
"Dimitris Spingos (Δημήτρης Σπίγγος) <dmtrs32@gmail.com>, 2011-2015.\n"
"Dorji Tashi <dorjee_doss@hotmail.com>, 2006.\n"
"Duarte Loreto <happyguy_pt@hotmail.com> 2002, 2003 (Maintainer).\n"
"Elias Norberg <elno0959 at student.su.se>, 2009.\n"
"Equipe de Tradução Inkscape Brasil <www.inkscapebrasil.org>, 2007.\n"
"Fatih Demir <kabalak@gtranslator.org>, 2000.\n"
"Firas Hanife <FirasHanife@gmail.com>, 2014-2016.\n"
"Foppe Benedictus <foppe.benedictus@gmail.com>, 2007-2009.\n"
"Francesc Dorca <f.dorca@filnet.es>, 2003. Traducció sodipodi.\n"
"Francisco Javier F. Serrador <serrador@arrakis.es>, 2003.\n"
"Francisco Xosé Vázquez Grandal <fxvazquez@arrakis.es>, 2001.\n"
"Frederic Rodrigo <f.rodrigo free.fr>, 2004-2005.\n"
"Ganesh Murmu <g_murmu_in@yahoo.com>, 2014.\n"
"Ge'ez Frontier Foundation <locales@geez.org>, 2002.\n"
"George Boukeas <boukeas@gmail.com>, 2011.\n"
"Heiko Wöhrle <mail@heikowoehrle.de>, 2014.\n"
"Hleb Valoshka <375gnu@gmail.com>, 2008-2009.\n"
"Hizkuntza Politikarako Sailburuordetza <hizkpol@ej-gv.es>, 2005.\n"
"Ilia Penev <lichopicho@gmail.com>, 2006.\n"
"Ivan Masár <helix84@centrum.sk>, 2006-2014. \n"
"Ivan Řihošek <irihosek@seznam.cz>, 2014.\n"
"Iñaki Larrañaga <dooteo@euskalgnu.org>, 2006.\n"
"Jānis Eisaks <jancs@dv.lv>, 2012-2014.\n"
"Jeffrey Steve Borbón Sanabria <jeff_kerokid@yahoo.com>, 2005.\n"
"Jesper Öqvist <jesper@llbit.se>, 2010, 2011.\n"
"Joaquim Perez i Noguer <noguer@gmail.com>, 2008-2009.\n"
"Jörg Müller <jfm@ram-brand.de>, 2005.\n"
"Jeroen van der Vegt <jvdvegt@gmail.com>, 2003, 2005, 2008.\n"
"Jin-Hwan Jeong <yongdoria@gmail.com>, 2009.\n"
"Jonathan Ernst <jernst@users.sourceforge.net>, 2006.\n"
"Jordi Mas i Hernàndez <jmas@softcatala.org>, 2015.\n"
"Jose Antonio Salgueiro Aquino <developer@telefonica.net>, 2003.\n"
"Josef Vybiral <josef.vybiral@gmail.com>, 2005-2006.\n"
"Juarez Rudsatz <juarez@correio.com>, 2004.\n"
"Junichi Uekawa <dancer@debian.org>, 2002.\n"
"Jurmey Rabgay <jur_gay@yahoo.com>, 2006.\n"
"Kai Lahmann <kailahmann@01019freenet.de>, 2000.\n"
"Karl Ove Hufthammer <karl@huftis.org>, 2004, 2005.\n"
"KATSURAGAWA Naoki <naopon@private.email.ne.jp>, 2006.\n"
"Keld Simonsen <keld@dkuug.dk>, 2000, 2001.\n"
"Kenji Inoue <kenz@oct.zaq.ne.jp>, 2006-2007.\n"
"Khandakar Mujahidul Islam <suzan@bengalinux.org>, 2006.\n"
"Kingsley Turner <kingsley@maddogsbreakfast.com.au>, 2006.\n"
"Kitae <bluetux@gmail.com>, 2006.\n"
"Kjartan Maraas <kmaraas@gnome.org>, 2000-2002.\n"
"Kris De Gussem <Kris.DeGussem@gmail.com>, 2008-2015.\n"
"Lauris Kaplinski <lauris@ariman.ee>, 2000.\n"
"Leandro Regueiro <leandro.regueiro@gmail.com>, 2006-2008, 2010.\n"
"Liu Xiaoqin <liuxqsmile@gmail.com>, 2008.\n"
"Louni Kandulna <kandulna.louni@gmail.com>, 2014.\n"
"Luca Bruno <luca.br@uno.it>, 2005.\n"
"Lucas Vieites Fariña<lucas@codexion.com>, 2003-2013.\n"
"Mahesh subedi <submanesh@hotmail.com>, 2006.\n"
"Marcin Floryan <marcin.floryan+inkscape (at) gmail.com>, 2016.\n"
"Maren Hachmann <marenhachmann@yahoo.com>, 2015-2016.\n"
"Martin Srebotnjak, <miles@filmsi.net>, 2005, 2010.\n"
"Masatake YAMATO <jet@gyve.org>, 2002.\n"
"Masato Hashimoto <cabezon.hashimoto@gmail.com>, 2009-2014.\n"
"Matiphas <matiphas _a_ free _point_ fr>, 2004-2006.\n"
"Mattias Hultgren <mattias_hultgren@tele2.se>, 2005, 2006.\n"
"Maxim Dziumanenko <mvd@mylinux.com.ua>, 2004.\n"
"Mətin Əmirov <metin@karegen.com>, 2003.\n"
"Mitsuru Oka <oka326@parkcity.ne.jp>, 2002.\n"
"Morphix User <pchitrakar@gmail.com>, 2006.\n"
"Mufit Eribol <meribol@ere.com.tr>, 2000.\n"
"Muhammad Bashir Al-Noimi <mhdbnoimi@gmail.com>, 2008.\n"
"Myckel Habets <myckel@sdf.lonestar.org>, 2008.\n"
"Nasreen <nasreen_saifee@hotmail.com>, 2013.\n"
"Nguyen Dinh Trung <nguyendinhtrung141@gmail.com>, 2007, 2008.\n"
"Nicolas Dufour <nicoduf@yahoo.fr>, 2008-2016.\n"
"Paresh prabhu <goa.paresh@Gmail.com>, 2013.\n"
"Pawan Chitrakar <pchitrakar@gmail.com>, 2006.\n"
"Przemysław Loesch <p_loesch@poczta.onet.pl>, 2005.\n"
"Quico Llach <quico@softcatala.org>, 2000. Traducció sodipodi.\n"
"Raymond Ostertag <raymond@linuxgraphic.org>, 2002, 2003.\n"
"Riku Leino <tsoots@gmail.com>, 2006-2011.\n"
"Rune Rønde Laursen <runerl@skjoldhoej.dk>, 2006.\n"
"Ruud Steltenpool <svg@steltenpower.com>, 2006.\n"
"Sangeeta <sk@gma.co>, 2011.\n"
"Savitha <savithasprasad@yahoo.co.in>, 2013.\n"
"Serdar Soytetir <sendirom@gmail.com>, 2005.\n"
"shivaken <shivaken@owls-nest.net>, 2004.\n"
"Shyam Krishna Bal <shyamkrishna_bal@yahoo.com>, 2006.\n"
"Simos Xenitellis <simos@hellug.gr>, 2001, 2011.\n"
"Spyros Blanas <cid_e@users.sourceforge.net>, 2006, 2011.\n"
"Stefan Graubner <pflaumenmus92@gmx.net>, 2005.\n"
"Supranee Thirawatthanasuk <supranee@opentle.org>, 2006.\n"
"Sushma Joshi <shshma_joshi8266@vsnl.net>, 2011.\n"
"Sveinn í Felli <sv1@fellsnet.is>, 2014-2015.\n"
"Sylvain Chiron <chironsylvain@orange.fr>, 2016.\n"
"Takeshi Aihana <aihana@muc.biglobe.ne.jp>, 2000, 2001.\n"
"Tim Sheridan <tghs@tghs.net>, 2007-2016.\n"
"Theppitak Karoonboonyanan <thep@linux.thai.net>, 2006.\n"
"Thiago Pimentel <thiago.merces@gmail.com>, 2006.\n"
"Toshifumi Sato <sato@centrosystem.com>, 2005.\n"
"Jon South <striker@lunar-linux.org>, 2006. \n"
"Uwe Schöler <oss@oss-marketplace.com>, 2006-2014.\n"
"Valek Filippov <frob@df.ru>, 2000, 2003.\n"
"Victor Dachev <vdachev@gmail.com>, 2006.\n"
"Victor Westmann <victor.westmann@gmail.com>, 2011, 2014.\n"
"Ville Pätsi, 2013.\n"
"Vincent van Adrighem <V.vanAdrighem@dirck.mine.nu>, 2003.\n"
"Vital Khilko <dojlid@mova.org>, 2003.\n"
"Vitaly Lipatov <lav@altlinux.ru>, 2002, 2004.\n"
"vonHalenbach <vonHalenbach@users.sourceforge.net>, 2005.\n"
"vrundeshw <vrundeshw@cdac.in>, 2012.\n"
"Waluyo Adi Siswanto <was.uthm@gmail.com>, 2011.\n"
"Wang Li <charlesw1234@163.com>, 2002.\n"
"Wei-Lun Chao <william.chao@ossii.com.tw>, 2006.\n"
"Wolfram Strempfer <wolfram@strempfer.de>, 2006.\n"
"Xavier Conde Rueda <xavi.conde@gmail.com>, 2004-2008.\n"
"Yaron Shahrabani <sh.yaron@gmail.com>, 2009.\n"
"Yukihiro Nakai <nakai@gnome.gr.jp>, 2000, 2003.\n"
"Yuri Beznos <zhiz0id@gmail.com>, 2006.\n"
"Yuri Chornoivan <yurchor@ukr.net>, 2007-2014.\n"
"Yuri Syrota <rasta@renome.rovno.ua>, 2000.\n"
"Yves Guillou <yvesguillou@users.sourceforge.net>, 2004.\n"
"Zdenko Podobný <zdpo@mailbox.sk>, 2003, 2004."
    ;
    
    translators_text.append(allTranslators);

    set_translator_credits(translators_text);
    set_license("GPL 3.0 or later"); // Overriden in next line
    set_license_type(Gtk::LICENSE_GPL_3_0);
}

void AboutBox::on_response(int response_id) {
    hide();
}
} // namespace Dialog
} // namespace UI
} // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
