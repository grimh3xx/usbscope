// Gerado por tools/gen_banner.py — não edite à mão.
// Logo: 75x48 blocos, 88 cores.

static constexpr int kLogoW = 75;
static constexpr int kLogoH = 48;
static constexpr int kIconEnd = 30;      // linhas [0, kIconEnd) = ícone
static constexpr int kWordStart = 32;  // linhas [kWordStart, kLogoH) = 'grimh3xx'
static constexpr int kSolutionsCol = 13;

static const char kPaletteChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!#$%&()*+,-./:;<=>@[]^_`{|";
static const unsigned int kPalette[] = {
    0x0028DC, 0x0028F0, 0x0028FF, 0x003CDC, 0x003CF0, 0x003CFF,
    0x0050DC, 0x0050F0, 0x0050FF, 0x0064F0, 0x0064FF, 0x008CF0,
    0x008CFF, 0x00A0F0, 0x00A0FF, 0x00B4F0, 0x00B4FF, 0x00C8F0,
    0x00C8FF, 0x14283C, 0x1428F0, 0x143C50, 0x143CF0, 0x145050,
    0x145064, 0x145078, 0x1450DC, 0x1450F0, 0x146478, 0x14648C,
    0x1464DC, 0x1464F0, 0x1464FF, 0x14788C, 0x1478A0, 0x148CA0,
    0x148CB4, 0x148CC8, 0x148CDC, 0x148CF0, 0x148CFF, 0x14A0C8,
    0x14A0DC, 0x14A0F0, 0x14A0FF, 0x14B4DC, 0x14B4F0, 0x14B4FF,
    0x14C8F0, 0x14C8FF, 0x283CF0, 0x2864F0, 0x288CDC, 0x288CF0,
    0x288CFF, 0x28A0C8, 0x28A0DC, 0x28A0F0, 0x28B4DC, 0x28B4F0,
    0x28C8F0, 0x28C8FF, 0x3C3C3C, 0x3C3C50, 0x505050, 0x505064,
    0x506464, 0x646464, 0x646478, 0x647878, 0x787878, 0x78788C,
    0x8C8C8C, 0x8C8CA0, 0x8CA0A0, 0xA0A0A0, 0xA0A0B4, 0xA0B4C8,
    0xB4B4C8, 0xB4C8C8, 0xC8C8C8, 0xC8C8DC, 0xC8DCDC, 0xDCDCDC,
    0xDCDCF0, 0xDCF0F0, 0xDCF0FF, 0xF0F0FF,
};

static const char* const kLogo[] = {
    "                                vSRSRSS8                                   ",
    "                              7vvSR  SS8w8                                 ",
    "                            nONNP     PSRSxw                               ",
    "                          1onONN   w    RSxw8                              ",
    "                         oMLLL    SSSS    RRSSx                            ",
    "                       zgoML    uQSSSSRw   RSSx                            ",
    "                     zKKJJ    rOQPPPPRSSR    QS                            ",
    "                     zKKJ   5srOQPP  RSSRSw  QS                            ",
    "                     fIG   ooonOr      RRSR  NP                            ",
    "                     fIG  1ooon         RSR  N     ww                      ",
    "                     WFD  oon             t    wRRw w                      ",
    "                     WFD  oo                  Rw  www                      ",
    "                     WCA  gg                 Pw                            ",
    "                     WCA  gg                uP                             ",
    "                     UCA  Kg   ffnnnnnnnnrrNq     u                        ",
    "                     UCA  Kg                  quPt t                       ",
    "                     UCA  KK        1110mmmqqru  uuu                       ",
    "                     UCA  KKe       1110mmmqqr                             ",
    "                     UCA  HIIH      oo2                                    ",
    "                     UCA  HIIHfe    oo2 mnnnnrr   uP                       ",
    "                     yCBB   HHIIf   ggn       rrrNr l                      ",
    "                     yCBBA   HIIfg  ggn        rrNrrl                      ",
    "                       UUCCB   HHH  gff    g0                              ",
    "                        UCCBBA   H  gf   zfg0                              ",
    "                          UUBCB     a   IIHg                               ",
    "                            BCBCA     bbIIH                                ",
    "                              BBCBB WFFEEG                                 ",
    "                               BCBBBWFFE                                   ",
    "                                 BBBBA                                     ",
    "                                   BB                                      ",
    "                                                                           ",
    "                                                                           ",
    "                      -[$                                                  ",
    "                      _|:             V6i       X878866                    ",
    "                      !)              Y9k       V646499                    ",
    "   !                                  X9j           d9d                    ",
    " :||[-{+  >]){|_)   `{__*   $_.|[/|^! Y9kj99l      i9c    86   68  d9j  Y9p",
    "&|_-:_|,  [|^:;||   ==]|,   $|/(|-(|/ Y993j89d    Y97Y    h9j l9d   69V 99 ",
    "/{)  /{,  @|%  :|+    ){+   ${( {# {; Y94  Y93    Y9994    49h94     94j9d ",
    ":|$  *|,  @|   &@(    ,|+   $|) |$ |: Y9k   94       39c   T998      d993  ",
    ":|$  *|,  @|          +|+   $|) |$ |: Y9k   94       Y9j    494       99d  ",
    "/{)  /|,  @|          +|+   $|) |$ |: Y9k   94  XY   V9k   d989c     4999  ",
    "&|_-:_|,  @{          ,{,   ${) {$ {: Y9k   94  39c  i9i   97 89    d9iY93 ",
    " :||[.|,  [|       )_^{|{^^ $|* |$ |; Y9k   96  V994499   69Y Z94  V98  p9d",
    "     ({,  :@       ([[[@[[[ !@& @! @- V4h   4k   X399lT  Y4i   j4Y j4Y   33",
    "     <|(                                                                   ",
    " (||||[                                                                    ",
    " $;;;,                                                                     ",
};
