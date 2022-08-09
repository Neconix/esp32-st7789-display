/**
 * @brief RGB565 colors 16-bit
 *
 */
#define RED                     0xF800
#define GREEN                   0x07E0
#define BLUE                    0x001F
#define BLACK                   0x0000
#define WHITE                   0xFFFF
#define GRAY                    0x8C51
#define YELLOW                  0xFFE0
#define CYAN                    0x07FF
#define PURPLE                  0xF81F

/**
 * @brief Web colors from extended colors table, see https://en.wikipedia.org/wiki/Web_colors#Extended_colors
 *
 * Size: 24-bit
 */

/* Pink colors */
#define WEB_MEDIUM_VIOLET_RED       0xC71585
#define WEB_DEEP_PINK               0xFF1493
#define WEB_PALE_VIOLET_RED         0xDB7093
#define WEB_HOT_PINK                0xFF69B4
#define WEB_LIGHT_PINK              0xFFB6C1
#define WEB_PINK                    0xFFC0CB

/* Red colors */
#define WEB_DARK_RED                0x8B0000
#define WEB_RED                     0xFF0000
#define WEB_FIRE_BRICK              0xB22222
#define WEB_CRIMSON                 0xDC143C
#define WEB_INDIAN_RED              0xCD5C5C
#define WEB_LIGHT_CORAL             0xF08080
#define WEB_SALMON                  0xFA8072
#define WEB_DARK_SALMON             0xE9967A
#define WEB_LIGHT_SALMON            0xFFA07A

/* Orange colors */
#define WEB_ORANGE_RED              0xFF4500
#define WEB_TOMATO                  0xFF6347
#define WEB_DARK_ORANGE             0xFF8C00
#define WEB_CORAL                   0xFF7F50
#define WEB_ORANGE                  0xFFA500

/* Yellow colors */
#define WEB_DARK_KHAKI              0xBDB76B
#define WEB_GOLD                    0xFFD700
#define WEB_KHAKI                   0xF0E68C
#define WEB_PEACH_PUFF              0xFFDAB9
#define WEB_YELLOW                  0xFFFF00
#define WEB_PALE_GOLDENROD          0xEEE8AA
#define WEB_MOCCASIN                0xFFE4B5
#define WEB_PAPAYA_WHIP             0xFFEFD5
#define WEB_LIGHT_GOLDENROD_YELLOW  0xFAFAD2
#define WEB_LEMON_CHIFFON           0xFFFACD
#define WEB_LIGHT_YELLOW            0xFFFFE0

/* Brown colors  */
#define WEB_MAROON                  0x800000
#define WEB_BROWN                   0xA52A2A
#define WEB_SADDLE_BROWN            0x8B4513
#define WEB_SIENNA                  0xA0522D
#define WEB_CHOCOLATE               0xD2691E
#define WEB_DARK_GOLDENROD          0xB8860B
#define WEB_PERU                    0xCD853F
#define WEB_ROSY_BROWN              0xBC8F8F
#define WEB_GOLDENROD               0xDAA520
#define WEB_SANDY_BROWN             0xF4A460
#define WEB_TAN                     0xD2B48C
#define WEB_BURLYWOOD               0xDEB887
#define WEB_WHEAT                   0xF5DEB3
#define WEB_NAVAJO_WHITE            0xFFDEAD
#define WEB_BISQUE                  0xFFE4C4
#define WEB_BLANCHED_ALMOND         0xFFEBCD
#define WEB_CORNSILK                0xFFF8DC

/* Green colors */
#define WEB_DARK_GREEN              0x006400
#define WEB_GREEN                   0x008000
#define WEB_DARK_OLIVE_GREEN        0x556B2F
#define WEB_FOREST_GREEN            0x228B22
#define WEB_SEA_GREEN               0x2E8B57
#define WEB_OLIVE                   0x808000
#define WEB_OLIVE_DRAB              0x6B8E23
#define WEB_MEDIUM_SEA_GREEN        0x3CB371
#define WEB_LIME_GREEN              0x32CD32
#define WEB_LIME                    0x00FF00
#define WEB_SPRING_GREEN            0x00FF7F
#define WEB_MEDIUM_SPRING_GREEN     0x00FA9A
#define WEB_DARK_SEA_GREEN          0x8FBC8F
#define WEB_MEDIUM_AQUAMARINE       0x66CDAA
#define WEB_YELLOW_GREEN            0x9ACD32
#define WEB_LAWN_GREEN              0x7CFC00
#define WEB_CHARTREUSE              0x7FFF00
#define WEB_LIGHT_GREEN             0x90EE90
#define WEB_GREEN_YELLOW            0xADFF2F
#define WEB_PALE_GREEN              0x98FB98

/* Cyan colors */
#define WEB_TEAL                    0x008080
#define WEB_DARK_CYAN               0x008B8B
#define WEB_LIGHT_SEA_GREEN         0x20B2AA
#define WEB_CADET_BLUE              0x5F9EA0
#define WEB_DARK_TURQUOISE          0x00CED1
#define WEB_MEDIUM_TURQUOISE        0x48D1CC
#define WEB_TURQUOISE               0x40E0D0
#define WEB_AQUA                    0x00FFFF
#define WEB_CYAN                    0x00FFFF
#define WEB_AQUAMARINE              0x7FFFD4
#define WEB_PALE_TURQUOISE          0xAFEEEE
#define WEB_LIGHT_CYAN              0xE0FFFF

 /* Blue colors */
#define WEB_MIDNIGHT_BLUE           0x191970
#define WEB_NAVY                    0x000080
#define WEB_DARK_BLUE               0x00008B
#define WEB_MEDIUM_BLUE             0x0000CD
#define WEB_BLUE                    0x0000FF
#define WEB_ROYAL_BLUE              0x4169E1
#define WEB_STEEL_BLUE              0x4682B4
#define WEB_DODGER_BLUE             0x1E90FF
#define WEB_DEEP_SKY_BLUE           0x00BFFF
#define WEB_CORNFLOWER_BLUE         0x6495ED
#define WEB_SKY_BLUE                0x87CEEB
#define WEB_LIGHT_SKY_BLUE          0x87CEFA
#define WEB_LIGHT_STEEL_BLUE        0xB0C4DE
#define WEB_LIGHT_BLUE              0xADD8E6
#define WEB_POWDER_BLUE             0xB0E0E6

/* Purple, violet, and magenta colors */
#define WEB_INDIGO                  0x4B0082
#define WEB_PURPLE                  0x800080
#define WEB_DARK_MAGENTA            0x8B008B
#define WEB_DARK_VIOLET             0x9400D3
#define WEB_DARK_SLATE_BLUE         0x483D8B
#define WEB_BLUE_VIOLET             0x8A2BE2
#define WEB_DARK_ORCHID             0x9932CC
#define WEB_FUCHSIA                 0xFF00FF
#define WEB_MAGENTA                 0xFF00FF
#define WEB_SLATE_BLUE              0x6A5ACD
#define WEB_MEDIUM_SLATEBLUE        0x7B68EE
#define WEB_MEDIUM_ORCHID           0xBA55D3
#define WEB_MEDIUM_PURPLE           0x9370DB
#define WEB_ORCHID                  0xDA70D6
#define WEB_VIOLET                  0xEE82EE
#define WEB_PLUM                    0xDDA0DD
#define WEB_THISTLE                 0xD8BFD8
#define WEB_LAVENDER                0xE6E6FA

/* White colors */
#define WEB_MISTY_ROSE              0xFFE4E1
#define WEB_ANTIQUE_WHITE           0xFAEBD7
#define WEB_LINEN                   0xFAF0E6
#define WEB_BEIGE                   0xF5F5DC
#define WEB_WHITE_SMOKE             0xF5F5F5
#define WEB_LAVENDER_BLUSH          0xFFF0F5
#define WEB_OLD_LACE                0xFDF5E6
#define WEB_ALICE_BLUE              0xF0F8FF
#define WEB_SEASHELL                0xFFF5EE
#define WEB_GHOST_WHITE             0xF8F8FF
#define WEB_HONEYDEW                0xF0FFF0
#define WEB_FLORAL_WHITE            0xFFFAF0
#define WEB_AZURE                   0xF0FFFF
#define WEB_MINTCREAM               0xF5FFFA
#define WEB_SNOW                    0xFFFAFA
#define WEB_IVORY                   0xFFFFF0
#define WEB_WHITE                   0xFFFFFF

/* Gray and black colors */
#define WEB_BLACK                   0x000000
#define WEB_DARK_SLATE_GRAY         0x2F4F4F
#define WEB_DIM_GRAY                0x696969
#define WEB_SLATE_GRAY              0x708090
#define WEB_GRAY                    0x808080
#define WEB_LIGHT_SLATE_GRAY        0x778899
#define WEB_DARK_GRAY               0xA9A9A9
#define WEB_SILVER                  0xC0C0C0
#define WEB_LIGHT_GRAY              0xD3D3D3
#define WEB_GAINSBORO               0xDCDCDC