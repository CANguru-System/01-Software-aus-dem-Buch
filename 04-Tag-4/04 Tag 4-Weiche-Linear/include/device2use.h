
/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <CANguru-Buch@web.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gustav Wostrack
 * ----------------------------------------------------------------------------
 */

// Alle Servodecoder benutzen den gleichen Quelltext.
// An den Stellen, an denen die Decoder sich unterscheiden, werden Anteile des Quelltextes
// aus- oder eingeblendet.
// Dies geschieht durch sogenannte Compiler-Direktiven, die durch #ifdef eingeleitet werden
// immer dann, wenn das auf #ifdef folgende Wort vorab definiert wurde (durch die Direktive #define),
// wird der durch die #ifdef Direktive eingeschlossene Quelltext übersetzt und wird Bestandteil
// des Programmes. Ansosnsten wird er ausgeblendet.
// Im Folgenden werden #define-Direktiven durch Kommentarzeichen ein- oder ausgeschaltet.

// Bei einem Signal ist nur formsignal definiert
//#define formsignal
// Bei einem normalen Armservo ist nur armservo definiert
//#define armservo
// Bei einem Linearservo ist nur linearservo definiert
#define linearservo
