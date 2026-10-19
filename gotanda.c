/*
 * SECTOR HEAVEN - 五反田三郎の事件簿 -  a top-down detective adventure for MeOS (M5Stack Tab5).
 *
 * Europa, Jupiter's moon.  In the golden-gai district "Sector Heaven", private eye Saburo Gotanda looks for a missing
 * Yashiki cat (a cat with koi-carp patches), fights yakuza, drunks and hostesses, and uncovers the secret of the city.
 * His partner is ARTEMIS, a quantum intelligence in the shape of his sunglasses; given raw material she can become anything.
 *
 *   arrows / WASD  walk        Space  talk / swing the baton        1  baton    2  ice bridge    3  metal shield
 *   H  hint from Artemis       Enter  next page                     Q  quit
 *
 * Screen: 80 px HUD + 20x10 tiles of 64 px.  Everything is drawn with fills and triangles, double buffered, 30 fps.
 */
#include "meos.h"
#include "meos_util.h"

#ifdef __wasm__
void *memset(void *d, int c, unsigned n) { volatile unsigned char *p = (volatile unsigned char *)d; while (n--) *p++ = (unsigned char)c; return d; }
void *memcpy(void *d, const void *s, unsigned n) { volatile unsigned char *p = (volatile unsigned char *)d; const unsigned char *q = (const unsigned char *)s; while (n--) *p++ = *q++; return d; }
#endif

#define SW 1280
#define SH 720
#define OY 80                 /* the HUD is 80 px high */
#define TS 64
#define RW 20
#define RH 10
#define NROOM 17
#define PS 40                 /* player size */
#define ES 44                 /* enemy size */
#define BS 64                 /* boss size */
#define MAXE 8
#define MAXP 8
#define MAXD 8
#define C(r, g, b) MEOS_RGB(r, g, b)

enum { UP, RIGHT, DOWN, LEFT };
enum { R_PLAZA = 5, R_OFFICE = 12, R_BAR, R_CLUB, R_HQ, R_BOSS };
enum { T_DRUNK, T_YAKUZA, T_HOSTESS, T_BOSS };
enum { N_CLIENT, N_BARTENDER, N_MAMA, N_ASTRO, N_DOCK, N_INFORMANT, N_VENDOR, N_TERMINAL, N_CAT };
enum { S_TITLE, S_PLAY, S_DIALOG, S_DEAD, S_CLEAR };
enum { A_NONE, A_CLIENT, A_BARPAY, A_MAMA, A_GATE, A_FUR, A_ENDING, A_ASTRO, A_TERM };

/* ---------- story text: first line = speaker, then up to 3 lines of at most 34 characters ---------- */

static const char *const D_intro[] = {
    "五反田\n雨は降らない。降るのは氷の粉とネオンの光だけ。\n木星の衛星エウロパ、ゴールデン街「セクターヘブン」。",
    "五反田\n俺は五反田三郎。この街でしがない私立探偵をやってる。\n今日の仕事は、迷い猫探しだ。",
    "アルテミス\nサブロー、ぼんやりしないでください。\n依頼人が事務所でお待ちですよ。",
    "五反田\nわかってるさ、アルテミス。\n相棒のサングラスは、いつも口うるさいな。",
    "アルテミス\n私は量子知能。素材さえあれば、どんな形にも\n姿を変えられます。頼ってくださいね。",
    "アルテミス\n1:警棒  2:氷の橋  3:金属の盾  H:ヒント\n氷の結晶を斬ると氷が、ヤクザからは金属が手に入ります。",
    0 };
static const char *const D_client1[] = {
    "依頼人・花子\n五反田さん、お願い。うちの「ミケ」を探して!\n錦鯉みたいに赤と黒のぶちが入った、ヤシキ猫なの。",
    "花子\nヤシキ猫は、この街でしか生まれない珍しい猫…。\nでも三日前、突然いなくなってしまったの。",
    "五反田\nわかった。足で稼ぐのが探偵の仕事だ。\nまずは酔いどれ横丁のバーで話を聞いてみよう。",
    "アルテミス\n情報収集ですね。バー「蛸」は広場の二つ東、\n酔いどれ横丁にあります。",
    0 };
static const char *const D_client2[] = { "花子\nミケを見つけたら、すぐに知らせてね。…お願い。", 0 };
static const char *const D_client3[] = {
    "花子\nああ、ミケ!本当にありがとう、五反田さん!",
    "アルテミス\n依頼完了ですね、サブロー。…ただ、私のデータにも\nあの「天国の門」の記録はありません。",
    "アルテミス\n私もヘブン計画で造られた存在のはず。\nいつか一緒に、氷の下の海を見に行きましょう。",
    0 };
static const char *const D_bar0[] = { "マスター\nいらっしゃい。…探偵さんか。\n悪いが、顔なじみ以外に話すことはないね。", 0 };
static const char *const D_barno[] = {
    "マスター\nヤシキ猫? 情報料は30円だ。",
    "マスター\n金がないなら、外の酔っ払いどもを\n軽くのしてこい。話はそれからだ。",
    0 };
static const char *const D_barpay[] = {
    "マスター\nヤシキ猫? 情報料は30円だ。",
    "五反田\n(30円を払った)",
    "マスター\n錦鯉柄の猫なら、三日前にクラブ「極光」の\nホステスが抱いていたよ。ママに聞いてみな。",
    "マスター\n会員制だが、俺の紹介だと言えば通してくれる。\nただし…おとなしい店じゃないぜ。",
    0 };
static const char *const D_bar2[] = {
    "マスター\n気をつけな。極光のホステスたちは\nただの接客係じゃない。ハイヒールが飛んでくるぞ。",
    "アルテミス\n飛び道具には金属の盾が有効です。3を押して。",
    0 };
static const char *const D_clublock[] = { "五反田\n「会員制」の札がかかっている。\n誰かの紹介がないと入れないようだ。", 0 };
static const char *const D_clubintro[] = {
    "ホステス\nあら、いらっしゃい探偵さん。\n今夜は「特別なお客様」以外、お断りなの!",
    "アルテミス\n敵意を検知。ハイヒールが来ます、サブロー!",
    0 };
static const char *const D_mama0[] = { "ママ・ルナ\nうちの子たちを倒してからにしてちょうだい!", 0 };
static const char *const D_mama1[] = {
    "ママ・ルナ\nやるじゃない…参ったわ。ミケちゃんのことなら話してあげる。",
    "ママ・ルナ\n黒龍会の組長が、ヤシキ猫を欲しがっているの。\n猫の毛並みの模様は、ただの模様じゃない。",
    "ママ・ルナ\n「天国の鍵」…この街の秘密を開く暗号なのよ。\nこれが黒龍会の通行証。気をつけてね。",
    "アルテミス\n通行証を入手。黒龍会の前庭は、広場の北東です。",
    0 };
static const char *const D_mama2[] = { "ママ・ルナ\n無事に帰ってきてね、探偵さん。", 0 };
static const char *const D_astro1[] = {
    "老天文学者\n見なさい、あの巨大な木星を。\nこの街は「天国」と呼ばれておるが…",
    "老天文学者\n本当はただの避難所じゃ。氷の下の海へ\n人類を逃がすための、大きな方舟なんじゃよ。",
    0 };
static const char *const D_astro2[] = {
    "老天文学者\nヤシキ猫を探しておるのか。\nあの猫たちは、方舟の「鍵」を継ぐ生き物じゃ。",
    0 };
static const char *const D_dock[] = {
    "港の男\nヤシキ猫なら見たぜ。黒い服の連中が、\n鯉柄の猫を籠に入れて運んでいった。",
    "港の男\n行き先は黒龍会の本部だ。…関わるなよ、あんた。",
    0 };
static const char *const D_info[] = {
    "情報屋\n黒龍会の組長・黒龍は、突進が得意だ。\nだが壁にぶつかると、しばらく動けなくなる。",
    "情報屋\nそこを叩け。…頼んだぜ、探偵さん。",
    0 };
static const char *const D_vendfull[] = { "屋台のおやじ\n元気そうじゃないか。おでんは要らないかい?", 0 };
static const char *const D_vendno[] = { "屋台のおやじ\nいらっしゃい! 50円でおでんが食えるよ。\n体力が全回復するぜ。…おや、金が足りないね。", 0 };
static const char *const D_vendok[] = { "屋台のおやじ\nまいどあり! 50円だよ。", "五反田\n(おでんを食べた。体力が全回復した!)", 0 };
static const char *const D_term[] = {
    "端末\n【ヘブン計画 記録 No.7】",
    "端末\n人類存続のため、ヤシキ猫の血統に\n「天国の門」の鍵を刻む。",
    "端末\n門はエウロパの海の底に通じている。\n鍵を持つ猫が最後の一匹になった時、門は開く。",
    "五反田\n最後の一匹…まさか、ミケが?",
    0 };
static const char *const D_fur[] = {
    "五反田\nこれは…ヤシキ猫の毛だ。赤と黒と白。\n間違いない、ミケはここを通ったな。",
    "アルテミス\n毛の成分を解析。ミケは黒龍会の方向へ\n運ばれています。急ぎましょう。",
    0 };
static const char *const D_gatelock[] = { "五反田\n黒龍会の本部だ。門は固く閉ざされている。\n通行証がいるな…", 0 };
static const char *const D_gateopen[] = { "五反田\n通行証を見せた。門が開く…行くぞ。", "アルテミス\n奥に多数の反応。気を引き締めて、サブロー。", 0 };
static const char *const D_factory[] = {
    "アルテミス\n亀裂の向こうに、ハートの形のエネルギー反応が。\n氷の橋をかけましょう。2を押して!",
    0 };
static const char *const D_bossintro[] = {
    "黒龍\n来たか、探偵。お前の探している猫は、\nそこの籠の中だ。",
    "黒龍\nこの街の「天国の門」を開く鍵だ。\n俺たちが先に、人類の未来を手に入れる!",
    "五反田\n人の飼い猫を、道具にするんじゃねえよ。\n依頼は依頼だ。返してもらうぜ!",
    0 };
static const char *const D_ending[] = {
    "黒龍\nぐっ…! だが、門は…もう…",
    "五反田\nミケ。迎えに来たぜ。…もう大丈夫だ。",
    "ミケ\nにゃあ。(背中の模様が、淡く光った)",
    "五反田\n鍵の秘密は、俺の胸にしまっておく。\n探偵は、依頼人の秘密を守るもんだ。",
    "アルテミス\n氷の下の海は、まだ眠ったまま。…今はまだ。",
    0 };
static const char *const D_hint0[] = { "アルテミス\n依頼人は事務所です。広場の左上の建物ですよ。", 0 };
static const char *const D_hint1[] = { "アルテミス\nバー「蛸」は酔いどれ横丁、広場の二つ東です。\n情報料は30円。酔っ払いから稼ぎましょう。", 0 };
static const char *const D_hint2[] = { "アルテミス\nクラブ極光は広場の東隣。ホステスは遠くから\n攻撃してきます。盾が有効です。", 0 };
static const char *const D_hint3[] = { "アルテミス\n黒龍会の前庭は北東です。通行証を門に。", 0 };
static const char *const D_hint4[] = { "アルテミス\n黒龍は突進します。壁にぶつかった隙を狙って!", 0 };
static const char *const D_hint5[] = { "アルテミス\n依頼人のもとへ戻りましょう。ミケも待っています。", 0 };
static const char *const D_noice[] = { "アルテミス\n素材が足りません。氷の結晶を斬ってください。", 0 };
static const char *const D_nometal[] = { "アルテミス\n金属が2つ必要です。ヤクザを倒すと手に入ります。", 0 };

static const char *const OBJECTIVE[] = {
    "依頼人に会おう(事務所)", "酔いどれ横丁のバーで情報を集めよう", "クラブ極光で聞き込みをしよう",
    "黒龍会の前庭へ。通行証を使おう", "組長・黒龍を倒し、ヤシキ猫を取り戻せ", "依頼人のもとへ戻ろう" };

/* ---------- world ---------- */

static char rm[NROOM][RH][RW + 1];
typedef struct { signed char r; char c; signed char x, y, w, h; } Cmd;
static const Cmd CMDS[] = {
    {0,'i',3,2,1,1},{0,'i',5,6,1,1},{0,'i',14,2,1,1},{0,'i',16,7,1,1},{0,'L',9,3,1,1},
    {1,'W',2,1,4,3},{1,'W',14,1,4,3},{1,'b',9,5,2,1},{1,'L',7,7,1,1},{1,'L',12,7,1,1},
    {2,'W',3,1,5,2},{2,'D',5,2,1,1},{2,'W',13,1,4,2},{2,'L',9,5,1,1},{2,'b',16,6,1,2},
    {3,'W',5,0,10,3},{3,'K',9,2,2,1},{3,'L',8,3,1,1},{3,'L',11,3,1,1},{3,'b',3,6,2,1},
    {4,'o',1,6,6,3},{4,'b',12,2,2,2},{4,'b',9,6,1,1},{4,'i',16,6,1,1},
    {5,'W',2,0,5,3},{5,'D',4,2,1,1},{5,'L',3,6,1,1},{5,'L',16,6,1,1},{5,'f',9,4,2,2},
    {6,'W',12,1,6,2},{6,'D',14,2,1,1},{6,'L',11,5,1,1},{6,'i',3,7,1,1},
    {7,'b',2,2,2,2},{7,'b',6,6,2,1},{7,'b',14,2,1,3},{7,'b',16,7,2,1},
    {8,'o',3,3,5,3},{8,'b',16,6,1,1},{8,'i',2,7,1,1},{8,'W',10,1,2,2},{8,'o',14,2,5,3},{8,'.',15,3,3,1},
    {9,'t',8,2,2,1},{9,'t',11,2,2,1},{9,'b',3,3,2,2},{9,'b',15,3,2,2},
    {10,'W',3,1,4,3},{10,'o',12,3,3,3},{10,'b',9,7,2,1},{10,'i',17,2,1,1},
    {11,'W',2,1,3,2},{11,'W',15,1,3,2},{11,'i',5,6,1,1},{11,'i',14,6,1,1},
    {12,'t',3,2,4,1},{12,'b',15,2,2,2},{12,'s',8,5,1,1},
    {13,'t',3,3,8,1},{13,'s',3,5,1,1},{13,'s',5,5,1,1},{13,'s',7,5,1,1},{13,'s',9,5,1,1},{13,'b',15,3,2,2},{13,'b',14,7,3,1},
    {14,'c',1,1,18,8},{14,'t',4,4,2,1},{14,'t',14,4,2,1},{14,'i',2,2,1,1},{14,'i',17,2,1,1},
    {15,'c',1,1,18,8},{15,'i',4,3,1,1},{15,'i',15,3,1,1},{15,'i',4,6,1,1},{15,'i',15,6,1,1},{15,'D',9,0,2,1},
    {16,'c',1,1,18,8},{16,'i',3,3,1,1},{16,'i',16,3,1,1},{16,'i',3,6,1,1},{16,'i',16,6,1,1},
    {12,'D',9,9,2,1},{13,'D',9,9,2,1},{14,'D',9,9,2,1},{15,'D',9,9,2,1},{16,'D',9,9,2,1},
};
typedef struct { signed char r, x, y, tr, tx, ty; } Warp;
static const Warp WARPS[] = {
    {5,4,2,12,9,8},{2,5,2,13,9,8},{6,14,2,14,9,8},{3,9,2,15,9,8},{3,10,2,15,9,8},{15,9,0,16,9,8},{15,10,0,16,9,8},
    {12,9,9,5,4,3},{12,10,9,5,4,3},{13,9,9,2,5,3},{13,10,9,2,5,3},{14,9,9,6,14,3},{14,10,9,6,14,3},
    {15,9,9,3,9,3},{15,10,9,3,9,3},{16,9,9,15,9,1},{16,10,9,15,9,1},
};
typedef struct { signed char r, t, x, y; } Spawn;
static const Spawn SPAWNS[] = {
    {0,T_DRUNK,10,6},{1,T_DRUNK,5,6},{1,T_DRUNK,14,6},{2,T_DRUNK,10,6},{2,T_HOSTESS,8,7},{2,T_DRUNK,15,5},
    {3,T_YAKUZA,4,5},{3,T_YAKUZA,15,5},{3,T_YAKUZA,9,7},{4,T_DRUNK,10,3},{4,T_DRUNK,16,3},
    {6,T_HOSTESS,5,5},{6,T_DRUNK,9,7},{6,T_HOSTESS,15,6},{7,T_YAKUZA,10,4},{7,T_YAKUZA,14,6},{7,T_DRUNK,6,3},
    {8,T_YAKUZA,9,2},{8,T_YAKUZA,15,6},{8,T_DRUNK,3,8},{9,T_DRUNK,5,6},{9,T_DRUNK,14,6},
    {10,T_HOSTESS,6,6},{10,T_HOSTESS,16,7},{10,T_YAKUZA,9,4},{11,T_YAKUZA,6,5},{11,T_YAKUZA,14,4},
    {14,T_HOSTESS,4,6},{14,T_HOSTESS,15,6},{14,T_HOSTESS,9,5},
    {15,T_YAKUZA,5,5},{15,T_YAKUZA,14,5},{15,T_YAKUZA,9,4},{15,T_YAKUZA,7,2},{16,T_BOSS,9,4},
};
typedef struct { signed char r, t, x, y; } Npc;
static const Npc NPCS[] = {
    {R_OFFICE,N_CLIENT,10,3},{R_BAR,N_BARTENDER,11,3},{R_CLUB,N_MAMA,9,2},{0,N_ASTRO,14,5},{4,N_DOCK,14,5},
    {7,N_INFORMANT,3,5},{9,N_VENDOR,10,2},{11,N_TERMINAL,10,2},{R_BOSS,N_CAT,9,1},
};
typedef struct { signed char r, x, y, t; } Item;       /* t: 0 heart container, 1 cat fur */
static const Item ITEMS[] = { {8,16,3,0}, {10,16,7,1} };

static void build(void)
{
    for (int r = 0; r < NROOM; r++) {
        for (int y = 0; y < RH; y++) {
            for (int x = 0; x < RW; x++) {
                char c = '.';
                if (x == 0 || y == 0 || x == RW - 1 || y == RH - 1) {
                    c = 'W';
                    if (r < 12) {
                        if (y == 0 && r >= 4 && x >= 8 && x <= 11) c = '.';
                        if (y == RH - 1 && r < 8 && x >= 8 && x <= 11) c = '.';
                        if (x == 0 && (r & 3) > 0 && y >= 4 && y <= 6) c = '.';
                        if (x == RW - 1 && (r & 3) < 3 && y >= 4 && y <= 6) c = '.';
                    }
                }
                rm[r][y][x] = c;
            }
            rm[r][y][RW] = 0;
        }
    }
    for (unsigned i = 0; i < sizeof(CMDS) / sizeof(CMDS[0]); i++) {
        const Cmd *m = &CMDS[i];
        for (int y = m->y; y < m->y + m->h; y++) for (int x = m->x; x < m->x + m->w; x++) rm[(int)m->r][y][x] = m->c;
    }
}

/* ---------- state ---------- */

typedef struct { int type, alive, x, y, hp, t, dir, hurt, state, hitid, sz; } Enemy;
typedef struct { int alive, x, y, vx, vy; } Proj;
typedef struct { int alive, x, y, kind; } Drop;       /* 0 money, 1 heart, 2 metal */

static int state, tick, state_t, room, prog, money, ice, metal, hp, maxhp, got, seen, gate_open, club_clear, boss_dead;
static int px, py, face, hold_dir, hold_t, step, inv_t, atk_t, atk_cd, swing, shield_t, astro_n, lock_msg, shake;
static Enemy en[MAXE];
static Proj pr[MAXP];
static Drop dr[MAXD];

static const char *const *dlg;
static int dlg_i, dlg_chars, dlg_act;

static int hash(int a, int b, int c) { unsigned h = (unsigned)(a * 73856093) ^ (unsigned)(b * 19349663) ^ (unsigned)(c * 83492791); h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15; return (int)(h & 0x7fff); }
static int rnd(int n) { return (int)(meos_rand() % (unsigned)n); }
static int iabs(int v) { return v < 0 ? -v : v; }

static int tile_solid(int tx, int ty, int edge)
{
    if (tx < 0 || ty < 0 || tx >= RW || ty >= RH) return edge;
    char c = rm[room][ty][tx];
    return c == 'W' || c == 'i' || c == 'b' || c == 't' || c == 's' || c == 'o' || c == 'K' || c == 'L';
}

static int npc_rect(int i, int *x, int *y)
{
    if (NPCS[i].r != room) return 0;
    if (NPCS[i].t == N_CAT && boss_dead) return 0;
    *x = NPCS[i].x * TS + 8; *y = NPCS[i].y * TS + 8;
    return 1;
}

static int blocked(int x, int y, int w, int h, int edge)
{
    int x0 = x < 0 ? -1 : x / TS, x1 = (x + w - 1) < 0 ? -1 : (x + w - 1) / TS;
    int y0 = y < 0 ? -1 : y / TS, y1 = (y + h - 1) < 0 ? -1 : (y + h - 1) / TS;
    for (int ty = y0; ty <= y1; ty++) for (int tx = x0; tx <= x1; tx++) if (tile_solid(tx, ty, edge)) return 1;
    for (unsigned i = 0; i < sizeof(NPCS) / sizeof(NPCS[0]); i++) {
        int nx, ny;
        if (npc_rect(i, &nx, &ny) && x < nx + 48 && x + w > nx && y < ny + 48 && y + h > ny) return 1;
    }
    return 0;
}

/* ---------- dialogue ---------- */

static void say(const char *const *pages, int act) { dlg = pages; dlg_i = 0; dlg_chars = 0; dlg_act = act; state = S_DIALOG; hold_t = 0; }

static void do_act(int a)
{
    switch (a) {
    case A_CLIENT: if (prog < 1) prog = 1; break;
    case A_BARPAY: if (prog < 2) { prog = 2; money -= 30; } break;
    case A_MAMA: if (prog < 3) prog = 3; break;
    case A_GATE:
        if (prog < 4) prog = 4;
        gate_open = 1; rm[3][2][9] = 'D'; rm[3][2][10] = 'D';
        break;
    case A_FUR: break;
    case A_ASTRO: break;
    case A_ENDING: state = S_CLEAR; state_t = 0; return;
    default: break;
    }
}

static int utf8_bytes(const char *s, int nchars)          /* bytes in the first nchars characters */
{
    int i = 0;
    while (*s && nchars > 0) {
        unsigned char ch = (unsigned char)*s;
        int k = ch >= 0xF0 ? 4 : ch >= 0xE0 ? 3 : ch >= 0xC0 ? 2 : 1;
        s += k; i += k; nchars--;
    }
    return i;
}

static int utf8_len(const char *s, int bytes)
{
    int n = 0;
    for (int i = 0; i < bytes && s[i];) { unsigned char ch = (unsigned char)s[i]; i += ch >= 0xF0 ? 4 : ch >= 0xE0 ? 3 : ch >= 0xC0 ? 2 : 1; n++; }
    return n;
}

static void spawn_room(void)
{
    for (int i = 0; i < MAXE; i++) en[i].alive = 0;
    for (int i = 0; i < MAXP; i++) pr[i].alive = 0;
    for (int i = 0; i < MAXD; i++) dr[i].alive = 0;
    int n = 0;
    for (unsigned i = 0; i < sizeof(SPAWNS) / sizeof(SPAWNS[0]) && n < MAXE; i++) {
        const Spawn *s = &SPAWNS[i];
        if (s->r != room) continue;
        if (room == R_CLUB && club_clear) continue;
        if (s->t == T_BOSS && boss_dead) continue;
        Enemy *e = &en[n++];
        e->type = s->t; e->alive = 1; e->sz = s->t == T_BOSS ? BS : ES;
        e->x = s->x * TS + (TS - e->sz) / 2; e->y = s->y * TS + (TS - e->sz) / 2;
        e->hp = s->t == T_DRUNK ? 2 : s->t == T_YAKUZA ? 3 : s->t == T_HOSTESS ? 2 : 16;
        e->t = 30 + rnd(40); e->dir = rnd(4); e->hurt = 0; e->state = 0; e->hitid = 0;
    }
}

static void enter_room(void)
{
    spawn_room();
    if (room == 8 && !(seen & 1)) { seen |= 1; say(D_factory, A_NONE); }
    else if (room == R_CLUB && !club_clear && !(seen & 2)) { seen |= 2; say(D_clubintro, A_NONE); }
    else if (room == R_BOSS && !boss_dead && !(seen & 4)) { seen |= 4; say(D_bossintro, A_NONE); }
}

static void place(int tx, int ty) { px = tx * TS + (TS - PS) / 2; py = ty * TS + (TS - PS) / 2; }

static void new_game(void)
{
    build();
    prog = 0; money = 0; ice = 0; metal = 0; maxhp = 10; hp = maxhp; got = 0; seen = 0; gate_open = 0; club_clear = 0; boss_dead = 0;
    astro_n = 0; room = R_PLAZA; place(9, 4); face = DOWN; hold_t = 0; inv_t = 0; atk_t = 0; atk_cd = 0; shield_t = 0; swing = 1;
    spawn_room();
    state = S_PLAY;
    say(D_intro, A_NONE);
}

static void respawn(void)
{
    hp = maxhp; room = R_PLAZA; place(9, 6); face = DOWN; inv_t = 60; hold_t = 0; atk_t = 0; shield_t = 0;
    spawn_room(); state = S_PLAY;
}

/* ---------- interaction ---------- */

static void talk(int i)
{
    switch (NPCS[i].t) {
    case N_CLIENT:
        if (prog == 0) say(D_client1, A_CLIENT);
        else if (prog >= 5) { say(D_client3, A_NONE); }
        else say(D_client2, A_NONE);
        break;
    case N_BARTENDER:
        if (prog == 0) say(D_bar0, A_NONE);
        else if (prog == 1) { if (money >= 30) say(D_barpay, A_BARPAY); else say(D_barno, A_NONE); }
        else say(D_bar2, A_NONE);
        break;
    case N_MAMA:
        if (!club_clear) say(D_mama0, A_NONE);
        else if (prog < 3) say(D_mama1, A_MAMA);
        else say(D_mama2, A_NONE);
        break;
    case N_ASTRO: say(astro_n++ == 0 ? D_astro1 : D_astro2, A_ASTRO); break;
    case N_DOCK: say(D_dock, A_NONE); break;
    case N_INFORMANT: say(D_info, A_NONE); break;
    case N_VENDOR:
        if (hp >= maxhp) say(D_vendfull, A_NONE);
        else if (money >= 50) { money -= 50; hp = maxhp; say(D_vendok, A_NONE); }
        else say(D_vendno, A_NONE);
        break;
    case N_TERMINAL: say(D_term, A_TERM); break;
    default: break;
    }
}

static void probe(int *x, int *y)                    /* the 40x40 spot in front of the player */
{
    *x = px; *y = py;
    if (face == UP) *y -= 44; else if (face == DOWN) *y += 44; else if (face == LEFT) *x -= 44; else *x += 44;
}

static int try_talk(void)
{
    int qx, qy;
    probe(&qx, &qy);
    for (unsigned i = 0; i < sizeof(NPCS) / sizeof(NPCS[0]); i++) {
        int nx, ny;
        if (!npc_rect(i, &nx, &ny)) continue;
        if (qx < nx + 56 && qx + PS > nx - 8 && qy < ny + 56 && qy + PS > ny - 8) { talk((int)i); return 1; }
    }
    return 0;
}

static void swing_start(void) { if (atk_cd > 0) return; atk_t = 8; atk_cd = 14; swing++; }

static void hit_rect(int *x, int *y, int *w, int *h)
{
    switch (face) {
    case UP: *x = px - 8; *y = py - 46; *w = 56; *h = 50; break;
    case DOWN: *x = px - 8; *y = py + 36; *w = 56; *h = 50; break;
    case LEFT: *x = px - 46; *y = py - 8; *w = 50; *h = 56; break;
    default: *x = px + 36; *y = py - 8; *w = 50; *h = 56; break;
    }
}

static void add_drop(int x, int y, int kind)
{
    for (int i = 0; i < MAXD; i++) if (!dr[i].alive) { dr[i].alive = 1; dr[i].x = x; dr[i].y = y; dr[i].kind = kind; return; }
}

static void kill_enemy(Enemy *e)
{
    e->alive = 0;
    int r = rnd(100), cx = e->x + e->sz / 2 - 14, cy = e->y + e->sz / 2 - 14;
    if (e->type == T_YAKUZA) { if (r < 45) add_drop(cx, cy, 2); else if (r < 75) add_drop(cx, cy, 0); else if (r < 90) add_drop(cx, cy, 1); }
    else if (e->type == T_BOSS) { }
    else { if (r < 60) add_drop(cx, cy, 0); else if (r < 78) add_drop(cx, cy, 1); }
    if (room == R_CLUB && !club_clear) {
        int left = 0;
        for (int i = 0; i < MAXE; i++) if (en[i].alive && en[i].type == T_HOSTESS) left++;
        if (!left) club_clear = 1;
    }
    if (e->type == T_BOSS) { boss_dead = 1; say(D_ending, A_ENDING); }
}

static int try_move(Enemy *e, int dx, int dy)          /* 1 if a wall stopped it */
{
    int hit = 0;
    if (dx) { if (!blocked(e->x + dx, e->y, e->sz, e->sz, 1)) e->x += dx; else hit = 1; }
    if (dy) { if (!blocked(e->x, e->y + dy, e->sz, e->sz, 1)) e->y += dy; else hit = 1; }
    return hit;
}

static void hurt_player(int dmg, int fromx, int fromy)
{
    if (inv_t > 0) return;
    if (shield_t > 0) { dmg = (dmg + 1) / 2; if (dmg < 1) dmg = 1; }
    hp -= dmg; inv_t = 45; shake = 6;
    int dx = (px + PS / 2) - fromx, dy = (py + PS / 2) - fromy;
    int sx = iabs(dx) >= iabs(dy) ? (dx < 0 ? -1 : 1) : 0, sy = iabs(dx) < iabs(dy) ? (dy < 0 ? -1 : 1) : 0;
    for (int i = 0; i < 4; i++) {
        if (!blocked(px + sx * 8, py + sy * 8, PS, PS, 0)) { px += sx * 8; py += sy * 8; }
    }
    if (hp <= 0) { hp = 0; state = S_DEAD; state_t = 0; }
}

static void shoot(Enemy *e)
{
    int ex = e->x + e->sz / 2, ey = e->y + e->sz / 2, dx = px + PS / 2 - ex, dy = py + PS / 2 - ey;
    int m = iabs(dx) > iabs(dy) ? iabs(dx) : iabs(dy);
    if (!m) return;
    for (int i = 0; i < MAXP; i++) if (!pr[i].alive) { pr[i].alive = 1; pr[i].x = ex - 10; pr[i].y = ey - 10; pr[i].vx = dx * 6 / m; pr[i].vy = dy * 6 / m; return; }
}

static void update_enemy(Enemy *e)
{
    if (e->hurt > 0) e->hurt--;
    int ex = e->x + e->sz / 2, ey = e->y + e->sz / 2, dx = px + PS / 2 - ex, dy = py + PS / 2 - ey;
    int dist = iabs(dx) + iabs(dy);
    switch (e->type) {
    case T_DRUNK:
        if (--e->t <= 0) { e->dir = rnd(5); e->t = 20 + rnd(40); }
        if (e->dir < 4) try_move(e, e->dir == RIGHT ? 2 : e->dir == LEFT ? -2 : 0, e->dir == DOWN ? 2 : e->dir == UP ? -2 : 0);
        break;
    case T_YAKUZA:
        if (dist < 440) {
            if ((tick >> 3 & 1) == 0 ? iabs(dx) > 4 : iabs(dy) <= 4) try_move(e, dx < 0 ? -3 : 3, 0);
            else try_move(e, 0, dy < 0 ? -3 : 3);
        } else {
            if (--e->t <= 0) { e->dir = rnd(5); e->t = 25 + rnd(40); }
            if (e->dir < 4) try_move(e, e->dir == RIGHT ? 1 : e->dir == LEFT ? -1 : 0, e->dir == DOWN ? 1 : e->dir == UP ? -1 : 0);
        }
        break;
    case T_HOSTESS:
        if (dist < 260) { try_move(e, dx < 0 ? 2 : -2, 0); try_move(e, 0, dy < 0 ? 2 : -2); }
        else if (dist > 460) { if (iabs(dx) > iabs(dy)) try_move(e, dx < 0 ? -2 : 2, 0); else try_move(e, 0, dy < 0 ? -2 : 2); }
        else if (tick & 4) try_move(e, dy < 0 ? -1 : 1, 0);
        if (--e->t <= 0) { shoot(e); e->t = 70 + rnd(40); }
        break;
    case T_BOSS:
        if (e->state == 0) {                                   /* stalk */
            if (iabs(dx) > iabs(dy)) try_move(e, dx < 0 ? -2 : 2, 0); else try_move(e, 0, dy < 0 ? -2 : 2);
            if (--e->t <= 0) { e->state = 1; e->t = 36; e->dir = iabs(dx) > iabs(dy) ? (dx < 0 ? LEFT : RIGHT) : (dy < 0 ? UP : DOWN); }
        } else if (e->state == 1) {                            /* telegraph */
            if (--e->t <= 0) e->state = 2;
        } else if (e->state == 2) {                            /* charge */
            int sx = e->dir == RIGHT ? 11 : e->dir == LEFT ? -11 : 0, sy = e->dir == DOWN ? 11 : e->dir == UP ? -11 : 0;
            if (try_move(e, sx, sy)) { e->state = 3; e->t = 80; shake = 10; }
        } else {                                               /* stunned */
            if (--e->t <= 0) { e->state = 0; e->t = 70 + rnd(40); }
        }
        break;
    }
    /* contact */
    if (e->alive && px < e->x + e->sz - 6 && px + PS > e->x + 6 && py < e->y + e->sz - 6 && py + PS > e->y + 6) {
        if (e->type == T_BOSS) { if (e->state != 3) hurt_player(e->state == 2 ? 4 : 3, ex, ey); }
        else hurt_player(e->type == T_DRUNK ? 1 : 2, ex, ey);
    }
}

static void player_attack(void)
{
    int hx, hy, hw, hh;
    hit_rect(&hx, &hy, &hw, &hh);
    for (int i = 0; i < MAXE; i++) {
        Enemy *e = &en[i];
        if (!e->alive || e->hitid == swing) continue;
        if (hx < e->x + e->sz && hx + hw > e->x && hy < e->y + e->sz && hy + hh > e->y) {
            e->hitid = swing; e->hurt = 8;
            int dmg = (e->type == T_BOSS && e->state == 3) ? 2 : 1;
            if (e->type == T_BOSS && e->state == 2) continue;
            e->hp -= dmg;
            int sx = face == RIGHT ? 1 : face == LEFT ? -1 : 0, sy = face == DOWN ? 1 : face == UP ? -1 : 0;
            if (e->type != T_BOSS) for (int k = 0; k < 3; k++) try_move(e, sx * 10, sy * 10);
            if (e->hp <= 0) kill_enemy(e);
        }
    }
    for (int ty = hy / TS; ty <= (hy + hh - 1) / TS; ty++) for (int tx = hx / TS; tx <= (hx + hw - 1) / TS; tx++)
        if (tx >= 0 && ty >= 0 && tx < RW && ty < RH && rm[room][ty][tx] == 'i') { rm[room][ty][tx] = '.'; if (ice < 9) ice++; }
}

static void use_bridge(void)
{
    int qx = px + PS / 2, qy = py + PS / 2;
    if (face == UP) qy -= 52; else if (face == DOWN) qy += 52; else if (face == LEFT) qx -= 52; else qx += 52;
    int tx = qx / TS, ty = qy / TS;
    if (qx < 0 || qy < 0 || tx >= RW || ty >= RH || rm[room][ty][tx] != 'o') return;
    if (ice < 1) { say(D_noice, A_NONE); return; }
    ice--; rm[room][ty][tx] = 'h';
    atk_cd = 10;
}

static void use_shield(void)
{
    if (metal < 2) { say(D_nometal, A_NONE); return; }
    metal -= 2; shield_t = 240;
}

static void update_play(void)
{
    if (inv_t > 0) inv_t--;
    if (atk_cd > 0) atk_cd--;
    if (shield_t > 0) shield_t--;
    if (shake > 0) shake--;
    /* walk */
    if (hold_t > 0 && atk_t == 0) {
        hold_t--;
        int dx = hold_dir == RIGHT ? 5 : hold_dir == LEFT ? -5 : 0, dy = hold_dir == DOWN ? 5 : hold_dir == UP ? -5 : 0;
        for (int s = 0; s < 5; s++) {
            int mx = dx ? (dx > 0 ? 1 : -1) : 0, my = dy ? (dy > 0 ? 1 : -1) : 0;
            if (!blocked(px + mx, py + my, PS, PS, 0)) { px += mx; py += my; }
            else break;
        }
        step++;
    }
    if (atk_t > 0) { atk_t--; player_attack(); }

    /* room edges */
    int cx = px + PS / 2, cy = py + PS / 2, moved = 0;
    if (room < 12) {
        if (cx < 0 && (room & 3) > 0) { room--; px += SW; moved = 1; }
        else if (cx >= SW && (room & 3) < 3) { room++; px -= SW; moved = 1; }
        else if (cy < 0 && room >= 4) { room -= 4; py += RH * TS; moved = 1; }
        else if (cy >= RH * TS && room < 8) { room += 4; py -= RH * TS; moved = 1; }
        if (moved) { enter_room(); return; }
        if (px < -PS / 2) px = -PS / 2;
        if (px > SW - PS / 2) px = SW - PS / 2;
        if (py < -PS / 2) py = -PS / 2;
        if (py > RH * TS - PS / 2) py = RH * TS - PS / 2;
    }

    /* doors */
    int tx = cx / TS, ty = cy / TS;
    if (tx >= 0 && ty >= 0 && tx < RW && ty < RH && rm[room][ty][tx] == 'D') {
        for (unsigned i = 0; i < sizeof(WARPS) / sizeof(WARPS[0]); i++) {
            const Warp *w = &WARPS[i];
            if (w->r != room || w->x != tx || w->y != ty) continue;
            if (room == 6 && w->tr == R_CLUB && prog < 2) { py += 30; hold_t = 0; say(D_clublock, A_NONE); return; }
            room = w->tr; place(w->tx, w->ty); hold_t = 0;
            enter_room();
            return;
        }
    }
    /* the locked gate */
    if (room == 3 && !gate_open && hold_t > 0 && hold_dir == UP) {
        int gx = (px + PS / 2) / TS, gy = (py - 6) / TS;
        if (py - 6 >= 0 && gx >= 0 && gx < RW && rm[3][gy][gx] == 'K') {
            if (prog >= 3) say(D_gateopen, A_GATE);
            else if (lock_msg <= 0) { lock_msg = 90; say(D_gatelock, A_NONE); }
        }
    }
    if (lock_msg > 0) lock_msg--;

    /* items and drops */
    for (unsigned i = 0; i < sizeof(ITEMS) / sizeof(ITEMS[0]); i++) {
        const Item *it = &ITEMS[i];
        if (it->r != room || (got >> i & 1)) continue;
        int ix = it->x * TS + 8, iy = it->y * TS + 8;
        if (px < ix + 48 && px + PS > ix && py < iy + 48 && py + PS > iy) {
            got |= 1 << i;
            if (it->t == 0) { maxhp += 2; hp = maxhp; }
            else say(D_fur, A_FUR);
        }
    }
    for (int i = 0; i < MAXD; i++) {
        if (!dr[i].alive) continue;
        if (px < dr[i].x + 28 && px + PS > dr[i].x && py < dr[i].y + 28 && py + PS > dr[i].y) {
            dr[i].alive = 0;
            if (dr[i].kind == 0) { money += 10; if (money > 999) money = 999; }
            else if (dr[i].kind == 1) { hp += 2; if (hp > maxhp) hp = maxhp; }
            else if (metal < 9) metal++;
        }
    }

    /* enemies and shots */
    for (int i = 0; i < MAXE; i++) if (en[i].alive) update_enemy(&en[i]);
    if (state != S_PLAY) return;
    for (int i = 0; i < MAXP; i++) {
        Proj *p = &pr[i];
        if (!p->alive) continue;
        p->x += p->vx; p->y += p->vy;
        int mx = p->x + 10, my = p->y + 10;
        if (mx < 0 || my < 0 || mx >= SW || my >= RH * TS || (rm[room][my / TS][mx / TS] != 'o' && tile_solid(mx / TS, my / TS, 1))) { p->alive = 0; continue; }
        if (px < p->x + 20 && px + PS > p->x && py < p->y + 20 && py + PS > p->y) {
            p->alive = 0;
            if (shield_t > 0) { shake = 3; continue; }
            hurt_player(2, p->x + 10 - p->vx * 4, p->y + 10 - p->vy * 4);
        }
    }
}

/* ---------- drawing ---------- */

static void rect(int x, int y, int w, int h, unsigned c)
{
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SW) w = SW - x;
    if (y + h > SH) h = SH - y;
    if (w > 0 && h > 0) meos_display_fill(x, y, w, h, c);
}
static int ox, oy;                                           /* screen shake */
static void R(int x, int y, int w, int h, unsigned c) { rect(x + ox, y + OY + oy, w, h, c); }
static void T(int x, int y, int x1, int y1, int x2, int y2, unsigned c) { meos_display_tri(x + ox, y + OY + oy, x1 + ox, y1 + OY + oy, x2 + ox, y2 + OY + oy, c); }

static void text(int x, int y, const char *s, int n, unsigned fg, unsigned bg, int sc)
{
    if (n < 0) n = meos_strlen(s);
    if (n <= 0 || n > 255) return;
    int cells = utf8_len(s, n);
    if (x < 0 || y < 0 || x + cells * 8 * sc > SW || y + 8 * sc > SH) return;
    meos_display_text(x, y, s, n, fg, bg, sc);
}

static int fmt_num(char *b, int n, int v)
{
    char d[8]; int k = 0;
    if (v < 0) v = 0;
    do { d[k++] = (char)('0' + v % 10); v /= 10; } while (v && k < 7);
    while (k) b[n++] = d[--k];
    return n;
}

static unsigned floor_color(void)
{
    switch (room) {
    case R_OFFICE: return C(150, 118, 80);
    case R_BAR: return C(96, 60, 36);
    case R_CLUB: return C(60, 22, 70);
    case R_HQ: return C(54, 70, 44);
    case R_BOSS: return C(60, 20, 24);
    default: return C(34, 44, 78);
    }
}

static void draw_tile(char c, int x, int y, int tx, int ty)
{
    int h = hash(tx, ty, room);
    switch (c) {
    case 'W':
        if (room >= 12) {
            R(x, y, TS, TS, C(70, 50, 66)); R(x, y + 31, TS, 2, C(40, 28, 40)); R(x + 30, y, 2, 32, C(40, 28, 40)); R(x + 14, y + 33, 2, 31, C(40, 28, 40));
        } else {
            unsigned neon = h % 3 == 0 ? C(255, 60, 160) : h % 3 == 1 ? C(60, 230, 255) : C(255, 220, 60);
            R(x, y, TS, TS, C(46, 34, 74));
            R(x + 8, y + 10, 18, 16, (h & 4) ? C(255, 215, 110) : C(26, 22, 50));
            R(x + 38, y + 10, 18, 16, (h & 8) ? C(255, 215, 110) : C(26, 22, 50));
            R(x + 8, y + 34, 18, 12, (h & 16) ? C(255, 215, 110) : C(26, 22, 50));
            R(x, y + TS - 6, TS, 4, neon);
        }
        break;
    case 'i':
        T(x + 32, y + 6, x + 12, y + 56, x + 52, y + 56, C(110, 200, 255));
        T(x + 32, y + 6, x + 32, y + 56, x + 52, y + 56, C(190, 240, 255));
        R(x + 12, y + 56, 40, 4, C(20, 30, 60));
        break;
    case 'b':
        R(x + 4, y + 6, 56, 54, C(150, 100, 50)); R(x + 4, y + 6, 56, 4, C(90, 55, 20)); R(x + 4, y + 56, 56, 4, C(90, 55, 20));
        R(x + 4, y + 30, 56, 4, C(90, 55, 20)); R(x + 30, y + 6, 4, 54, C(90, 55, 20));
        break;
    case 't': R(x, y + 8, TS, 48, C(120, 76, 40)); R(x, y + 8, TS, 8, C(180, 126, 70)); R(x, y + 56, TS, 4, C(60, 36, 16)); break;
    case 's': R(x + 18, y + 18, 28, 12, C(200, 60, 70)); R(x + 30, y + 30, 4, 22, C(170, 170, 180)); R(x + 20, y + 52, 24, 4, C(170, 170, 180)); break;
    case 'o':
        R(x, y, TS, TS, C(6, 8, 18)); R(x + 6, y + 6, TS - 12, TS - 12, C(12, 20, 44));
        R(x, y, TS, 4, C(90, 130, 190));
        break;
    case 'h':
        R(x, y, TS, TS, C(6, 8, 18)); R(x + 2, y + 4, TS - 4, TS - 8, C(170, 225, 255));
        R(x + 2, y + 20, TS - 4, 3, C(110, 170, 220)); R(x + 2, y + 40, TS - 4, 3, C(110, 170, 220));
        break;
    case 'L':
        R(x + 28, y + 24, 8, 36, C(120, 120, 140)); R(x + 18, y + 6, 28, 18, C(255, 230, 120)); R(x + 24, y + 10, 16, 10, C(255, 255, 210));
        break;
    case 'D':
        R(x + 6, y, 52, TS, C(255, 190, 80)); R(x + 14, y + 6, 36, TS - 6, C(60, 36, 14));
        break;
    case 'K':
        R(x, y, TS, TS, C(70, 20, 24));
        for (int k = 0; k < 4; k++) R(x + 6 + k * 16, y, 6, TS, C(190, 150, 60));
        R(x, y + 28, TS, 6, C(190, 150, 60));
        break;
    case 'c': R(x, y, TS, TS, room == R_CLUB ? C(130, 24, 90) : room == R_BOSS ? C(100, 20, 20) : C(88, 110, 60)); R(x, y, TS, 2, C(200, 160, 60)); R(x, y, 2, TS, C(200, 160, 60)); break;
    case 'f': R(x + 6, y + 18, 52, 26, C(60, 92, 150)); R(x + 14, y + 24, 20, 6, C(120, 160, 220)); break;
    default: break;
    }
}

static void person(int x, int y, int sz, int dir, int stp, unsigned body, unsigned legs, unsigned hair, unsigned skin, unsigned hatc, int shades, int longhair)
{
#define S(v) ((v) * sz / 48)
    R(x + S(12), y + S(34) + (stp ? S(2) : 0), S(10), S(13), legs);
    R(x + S(26), y + S(34) + (stp ? 0 : S(2)), S(10), S(13), legs);
    R(x + S(8), y + S(18), S(32), S(20), body);
    R(x + S(3), y + S(20), S(6), S(15), body); R(x + S(39), y + S(20), S(6), S(15), body);
    if (longhair) R(x + S(8), y + S(6), S(32), S(30), hair);
    R(x + S(12), y + S(2), S(24), S(19), dir == UP ? hair : skin);
    R(x + S(12), y + S(2), S(24), S(6), hair);
    if (dir != UP) {
        if (shades) {
            unsigned lens = shades == 2 ? C(70, 230, 255) : C(10, 10, 14);
            if (dir == DOWN) { R(x + S(14), y + S(9), S(9), S(6), lens); R(x + S(25), y + S(9), S(9), S(6), lens); R(x + S(22), y + S(10), S(4), S(2), lens); }
            else R(x + (dir == LEFT ? S(12) : S(20)), y + S(9), S(16), S(6), lens);
        } else {
            if (dir == DOWN) { R(x + S(17), y + S(11), S(4), S(4), C(0, 0, 0)); R(x + S(28), y + S(11), S(4), S(4), C(0, 0, 0)); }
            else R(x + (dir == LEFT ? S(14) : S(30)), y + S(11), S(4), S(4), C(0, 0, 0));
        }
    }
    if (hatc) { R(x + S(8), y + S(3), S(32), S(4), hatc); R(x + S(14), y - S(3), S(20), S(8), hatc); }
#undef S
}

static void draw_cat(int x, int y)
{
    R(x, y + 8, 40, 22, C(255, 255, 255));                       /* body */
    R(x + 6, y + 8, 12, 10, C(240, 70, 30)); R(x + 24, y + 14, 12, 12, C(30, 30, 36)); R(x + 14, y + 20, 10, 8, C(240, 70, 30));
    R(x + 34, y, 18, 18, C(255, 255, 255)); R(x + 38, y + 2, 10, 6, C(240, 70, 30));       /* head */
    T(x + 34, y + 2, x + 38, y - 8, x + 42, y + 2, C(30, 30, 36)); T(x + 46, y + 2, x + 50, y - 8, x + 52, y + 2, C(240, 70, 30));
    R(x + 42, y + 8, 3, 3, C(0, 0, 0)); R(x + 48, y + 8, 3, 3, C(0, 0, 0));
    R(x - 10, y + 10, 12, 5, C(255, 255, 255)); R(x - 10, y + 10, 5, 5, C(30, 30, 36));     /* tail */
    R(x + 4, y + 28, 6, 8, C(255, 255, 255)); R(x + 30, y + 28, 6, 8, C(255, 255, 255));
}

static void draw_npc(int i)
{
    int nx, ny;
    if (!npc_rect(i, &nx, &ny)) return;
    int x = nx - 0, y = ny - 4;
    switch (NPCS[i].t) {
    case N_CLIENT: person(x, y, 48, DOWN, 0, C(235, 130, 175), C(120, 60, 90), C(90, 40, 110), C(255, 214, 180), 0, 0, 1); break;
    case N_BARTENDER: person(x, y, 48, DOWN, 0, C(240, 240, 240), C(30, 30, 40), C(30, 24, 20), C(240, 190, 150), 0, 0, 0); R(x + 20, y + 20, 8, 5, C(220, 30, 40)); break;
    case N_MAMA: person(x, y, 48, DOWN, 0, C(150, 60, 200), C(100, 40, 140), C(250, 215, 90), C(255, 214, 180), 0, 0, 1); break;
    case N_ASTRO: person(x, y, 48, LEFT, 0, C(150, 150, 170), C(80, 80, 100), C(240, 240, 245), C(240, 200, 170), 0, 0, 0); break;
    case N_DOCK: person(x, y, 48, DOWN, 0, C(240, 140, 30), C(50, 60, 90), C(40, 30, 20), C(220, 170, 130), C(250, 220, 40), 0, 0); break;
    case N_INFORMANT: person(x, y, 48, DOWN, 0, C(70, 74, 86), C(40, 40, 50), C(30, 30, 30), C(230, 190, 150), C(110, 110, 120), 1, 0); break;
    case N_VENDOR: person(x, y, 48, DOWN, 0, C(250, 250, 250), C(60, 60, 80), C(30, 20, 20), C(240, 190, 150), C(220, 40, 40), 0, 0); break;
    case N_TERMINAL:
        R(x, y + 4, 48, 48, C(50, 56, 80)); R(x + 6, y + 10, 36, 24, C(10, 40, 50)); R(x + 10, y + 14, 28, 3, C(60, 255, 200)); R(x + 10, y + 22, 20, 3, C(60, 255, 200));
        R(x + 10, y + 38, 8, 6, C(255, 80, 80));
        break;
    case N_CAT:
        R(x - 6, y - 4, 60, 56, C(40, 30, 40));
        draw_cat(x - 2, y + 6);
        for (int k = 0; k < 6; k++) R(x - 6 + k * 11, y - 4, 4, 56, C(200, 190, 80));
        break;
    default: break;
    }
}

static void draw_enemy(Enemy *e)
{
    int x = e->x - 2, y = e->y - 6, flash = e->hurt > 0 && (tick & 1);
    int d = e->type == T_BOSS ? DOWN : (iabs(px - e->x) > iabs(py - e->y) ? (px < e->x ? LEFT : RIGHT) : (py < e->y ? UP : DOWN));
    int stp = (tick >> 3) & 1;
    if (flash) { R(e->x, e->y, e->sz, e->sz, C(255, 255, 255)); return; }
    switch (e->type) {
    case T_DRUNK:
        person(x + (stp ? 1 : -1), y, 48, d, stp, C(110, 150, 90), C(70, 70, 100), C(70, 40, 20), C(255, 150, 140), 0, 0, 0);
        R(x + 38, y + 24, 6, 14, C(60, 190, 80)); R(x + 39, y + 20, 4, 5, C(60, 190, 80));
        break;
    case T_YAKUZA:
        person(x, y, 48, d, stp, C(26, 26, 34), C(20, 20, 26), C(10, 10, 10), C(235, 190, 150), 0, 1, 0);
        R(x + 21, y + 20, 6, 14, C(210, 30, 40));
        break;
    case T_HOSTESS:
        person(x, y, 48, d, stp, C(245, 90, 170), C(200, 60, 130), C(250, 215, 90), C(255, 214, 180), 0, 0, 1);
        R(x + 30, y + 4, 6, 6, C(255, 60, 60));
        break;
    case T_BOSS: {
        unsigned tele = e->state == 1 && (tick & 2) ? C(255, 60, 40) : C(40, 20, 50);
        if (e->state == 3) tele = C(120, 120, 160);
        person(e->x - 4, e->y - 10, 74, DOWN, stp, tele, C(20, 20, 26), C(20, 20, 20), C(235, 190, 150), 0, 1, 0);
        T(e->x + 12, e->y + 30, e->x + 52, e->y + 30, e->x + 32, e->y + 54, C(60, 200, 90));      /* dragon tattoo */
        if (e->state == 1) R(e->x + 24, e->y - 24, 16, 14, C(255, 230, 60));
        break; }
    }
}

static void draw_player(void)
{
    int x = px - 4, y = py - 10;
    if (inv_t > 0 && (tick & 2)) return;
    person(x, y, 48, face, (step >> 2) & 1, C(180, 140, 90), C(60, 56, 70), C(50, 40, 40), C(240, 195, 155), C(80, 70, 80), 2, 0);
    if (shield_t > 0) {
        int fx = face == LEFT ? x - 12 : face == RIGHT ? x + 30 : x + 10, fy = face == UP ? y - 6 : y + 14;
        R(fx, fy, 24, 30, C(190, 200, 215)); R(fx + 4, fy + 4, 16, 22, C(120, 140, 170)); R(fx + 10, fy + 10, 4, 10, C(255, 255, 255));
        if (shield_t < 60 && (tick & 2)) R(fx, fy, 24, 30, C(255, 255, 255));
    }
    if (atk_t > 0) {
        int hx, hy, hw, hh;
        hit_rect(&hx, &hy, &hw, &hh);
        unsigned glow = C(90, 240, 255);
        if (face == UP || face == DOWN) { R(hx + 22, hy, 12, hh, glow); R(hx + 25, hy, 6, hh, C(255, 255, 255)); }
        else { R(hx, hy + 22, hw, 12, glow); R(hx, hy + 25, hw, 6, C(255, 255, 255)); }
    }
}

static void draw_heart(int x, int y, int full)                /* full: 0 empty, 1 half, 2 full */
{
    unsigned col = C(240, 40, 70), bg = C(70, 20, 40);
    rect(x, y, 10, 8, full ? col : bg); rect(x + 12, y, 10, 8, full == 2 ? col : bg);
    rect(x, y + 6, 11, 8, full ? col : bg); rect(x + 11, y + 6, 11, 8, full == 2 ? col : bg);
    meos_display_tri(x, y + 12, x + 11, y + 12, x + 11, y + 24, full ? col : bg);
    meos_display_tri(x + 11, y + 12, x + 22, y + 12, x + 11, y + 24, full == 2 ? col : bg);
}

static void draw_hud(void)
{
    rect(0, 0, SW, OY, C(8, 8, 20)); rect(0, OY - 3, SW, 3, C(255, 60, 160));
    for (int i = 0; i < maxhp / 2; i++) { int v = hp - i * 2; draw_heart(20 + i * 30, 8, v >= 2 ? 2 : v == 1 ? 1 : 0); }
    char b[48]; int n;
    n = 0; b[n++] = 'Y'; b[n++] = 'E'; b[n++] = 'N'; b[n++] = ' '; n = fmt_num(b, n, money); while (n < 10) b[n++] = ' ';
    text(380, 8, b, n, C(255, 230, 80), C(8, 8, 20), 2);
    n = 0; b[n++] = 'I'; b[n++] = 'C'; b[n++] = 'E'; b[n++] = ' '; n = fmt_num(b, n, ice); b[n++] = ' '; b[n++] = ' ';
    b[n++] = 'M'; b[n++] = 'E'; b[n++] = 'T'; b[n++] = 'A'; b[n++] = 'L'; b[n++] = ' '; n = fmt_num(b, n, metal); b[n++] = ' '; b[n++] = ' ';
    text(380, 30, b, n, C(150, 230, 255), C(8, 8, 20), 2);
    /* Artemis (sunglasses) */
    rect(740, 12, 20, 12, C(70, 230, 255)); rect(766, 12, 20, 12, C(70, 230, 255)); rect(760, 14, 6, 3, C(70, 230, 255));
    text(800, 8, "ARTEMIS", 7, C(70, 230, 255), C(8, 8, 20), 2);
    const char *form = shield_t > 0 ? "FORM: SHIELD" : atk_t > 0 ? "FORM: BATON " : "FORM: BATON ";
    text(740, 30, form, 12, C(200, 220, 255), C(8, 8, 20), 2);
    text(980, 8, "1 BATON 2 BRIDGE", 16, C(160, 160, 190), C(8, 8, 20), 2);
    text(980, 30, "3 SHIELD  H HINT", 16, C(160, 160, 190), C(8, 8, 20), 2);
    const char *o = OBJECTIVE[prog > 5 ? 5 : prog];
    text(20, 52, "目的:", -1, C(255, 230, 80), C(8, 8, 20), 2);
    text(20 + 5 * 16 + 6, 52, o, -1, C(255, 255, 255), C(8, 8, 20), 2);
}

static void draw_world(void)
{
    ox = shake ? (int)(tick & 1) * 6 - 3 : 0; oy = shake ? (int)((tick >> 1) & 1) * 6 - 3 : 0;
    rect(0, OY, SW, SH - OY, floor_color());
    if (room < 12) { for (int i = 1; i < RH; i++) R(0, i * TS, SW, 1, C(44, 56, 96)); for (int i = 1; i < RW; i++) R(i * TS, 0, 1, RH * TS, C(44, 56, 96)); }
    for (int ty = 0; ty < RH; ty++) for (int tx = 0; tx < RW; tx++) { char c = rm[room][ty][tx]; if (c != '.') draw_tile(c, tx * TS, ty * TS, tx, ty); }
    for (unsigned i = 0; i < sizeof(ITEMS) / sizeof(ITEMS[0]); i++) {
        const Item *it = &ITEMS[i];
        if (it->r != room || (got >> i & 1)) continue;
        int x = it->x * TS, y = it->y * TS + ((tick >> 3) & 1) * 3;
        if (it->t == 0) { R(x + 10, y + 12, 18, 14, C(255, 70, 100)); R(x + 32, y + 12, 18, 14, C(255, 70, 100)); R(x + 10, y + 22, 40, 12, C(255, 70, 100)); T(x + 10, y + 32, x + 50, y + 32, x + 30, y + 54, C(255, 70, 100)); R(x + 16, y + 16, 6, 6, C(255, 200, 210)); }
        else { R(x + 14, y + 26, 36, 12, C(255, 255, 255)); R(x + 18, y + 26, 10, 12, C(240, 70, 30)); R(x + 34, y + 26, 10, 12, C(30, 30, 36)); }
    }
    for (int i = 0; i < MAXD; i++) {
        if (!dr[i].alive) continue;
        int x = dr[i].x, y = dr[i].y;
        if (dr[i].kind == 0) { R(x + 4, y + 2, 20, 24, C(255, 210, 40)); R(x + 8, y + 6, 12, 16, C(255, 240, 140)); }
        else if (dr[i].kind == 1) { R(x + 2, y + 4, 10, 8, C(255, 70, 100)); R(x + 14, y + 4, 10, 8, C(255, 70, 100)); R(x + 2, y + 10, 22, 8, C(255, 70, 100)); T(x + 2, y + 16, x + 24, y + 16, x + 13, y + 28, C(255, 70, 100)); }
        else { R(x + 2, y + 8, 24, 16, C(170, 180, 200)); R(x + 6, y + 12, 16, 4, C(230, 240, 255)); }
    }
    for (unsigned i = 0; i < sizeof(NPCS) / sizeof(NPCS[0]); i++) draw_npc((int)i);
    for (int i = 0; i < MAXE; i++) if (en[i].alive) draw_enemy(&en[i]);
    for (int i = 0; i < MAXP; i++) if (pr[i].alive) { R(pr[i].x, pr[i].y, 20, 20, C(255, 80, 180)); R(pr[i].x + 5, pr[i].y + 5, 10, 10, C(255, 200, 230)); }
    draw_player();
    ox = oy = 0;
    draw_hud();
}

static void draw_dialog(void)
{
    const char *page = dlg[dlg_i];
    rect(40, 478, 1200, 226, C(255, 255, 255)); rect(44, 482, 1192, 218, C(8, 12, 44));
    int line = 0, nchars = dlg_chars, i = 0;
    int is_art = page[0] == (char)0xE3 && page[1] == (char)0x82 && page[2] == (char)0xA2;        /* ア... = Artemis */
    while (page[i] && line < 4) {
        int j = i;
        while (page[j] && page[j] != '\n') j++;
        int len = j - i, total = utf8_len(page + i, len);
        int show = nchars < total ? nchars : total;
        if (line == 0) {
            if (show > 0) text(64, 490, page + i, utf8_bytes(page + i, show), is_art ? C(70, 230, 255) : C(255, 230, 80), C(8, 12, 44), 3);
        } else if (show > 0) {
            text(64, 530 + (line - 1) * 54, page + i, utf8_bytes(page + i, show), C(255, 255, 255), C(8, 12, 44), 4);
        }
        nchars -= total;
        if (nchars < 0) nchars = 0;
        line++;
        i = page[j] ? j + 1 : j;
    }
    if ((tick >> 3) & 1) T(1196, 672 - OY, 1220, 672 - OY, 1208, 688 - OY, C(255, 230, 80));
}

static int page_chars(const char *page)
{
    int n = 0;
    for (int i = 0; page[i]; i++) if (page[i] == '\n') n++;
    return utf8_len(page, meos_strlen(page)) - n;
}

static void center(int y, const char *s, unsigned fg, unsigned bg, int sc)
{
    int w = utf8_len(s, meos_strlen(s)) * 8 * sc;
    text((SW - w) / 2, y, s, -1, fg, bg, sc);
}

static void draw_title(void)
{
    rect(0, 0, SW, SH, C(10, 8, 30));
    for (int i = 0; i < 40; i++) { int h = hash(i, 3, 9); rect((h * 7) % SW, (h * 13) % 380, 3, 3, C(220, 220, 255)); }
    rect(900, 60, 280, 280, C(230, 170, 110)); rect(920, 120, 240, 30, C(200, 120, 70)); rect(910, 200, 260, 24, C(250, 210, 160));   /* Jupiter */
    for (int i = 0; i < 9; i++) { int bh = 120 + hash(i, 1, 1) % 160; rect(i * 150 - 20, 500 - bh, 130, bh + 220, C(26 + i % 3 * 6, 20, 56)); rect(i * 150 - 20, 500 - bh, 130, 6, i % 2 ? C(255, 60, 160) : C(60, 230, 255)); for (int w = 0; w < bh / 40; w++) rect(i * 150 + 4, 520 - bh + w * 40, 18, 14, (hash(i, w, 5) & 1) ? C(255, 215, 110) : C(26, 22, 50)); }
    rect(0, 640, SW, 80, C(34, 44, 78)); rect(0, 640, SW, 4, C(255, 60, 160));
    rect(0, 96, SW, 200, C(10, 8, 30)); rect(0, 96, SW, 3, C(255, 60, 160)); rect(0, 293, SW, 3, C(60, 230, 255));
    rect(0, 410, SW, 130, C(10, 8, 30));
    center(110, "SECTOR HEAVEN", C(255, 255, 255), C(10, 8, 30), 8);
    center(200, "五反田三郎の事件簿 ー ヤシキ猫を探せ ー", C(255, 230, 80), C(10, 8, 30), 3);
    center(268, "木星の衛星エウロパ ゴールデン街", C(160, 200, 255), C(10, 8, 30), 2);
    person(560, 540, 96, DOWN, 0, C(180, 140, 90), C(60, 56, 70), C(50, 40, 40), C(240, 195, 155), C(80, 70, 80), 2, 0);
    draw_cat(700, 590);
    if ((tick >> 4) & 1) center(420, "PRESS ENTER", C(255, 255, 255), C(10, 8, 30), 4);
    center(490, "矢印/WASD:移動  Space:話す・攻撃  1/2/3:アルテミス変形  H:ヒント  Q:終了", C(200, 200, 230), C(10, 8, 30), 2);
}

/* ---------- main ---------- */

void _start(void)
{
    if (meos_display_open() < 0) { meos_puts("this app needs the display permission\n"); return; }
    meos_display_clip(0, 0, SW, SH);
    unsigned last = meos_uptime_ms();
    build();
    state = S_TITLE; state_t = 0; room = R_PLAZA;
#ifdef DEBUG_ROOM
    new_game(); room = DEBUG_ROOM; place(9, 6); spawn_room(); state = S_PLAY; prog = DEBUG_PROG; maxhp = 10; hp = 10; ice = 3; metal = 4; money = 80;
#endif
    for (;;) {
        tick++; state_t++;
        int k, quit = 0;
        while ((k = meos_getkey()) >= 0) {
            if (k == 'q' || k == 'Q') { quit = 1; break; }
            int enter = (k == '\r' || k == '\n'), space = (k == ' ');
            if (state == S_TITLE || state == S_DEAD) {
                if ((enter || space) && state_t > 15) { if (state == S_TITLE) new_game(); else respawn(); }
                continue;
            }
            if (state == S_CLEAR) { if ((enter || space) && state_t > 30) { state = S_TITLE; state_t = 0; } continue; }
            if (state == S_DIALOG) {
                if (enter || space || k == 'e') {
                    int total = page_chars(dlg[dlg_i]);
                    if (dlg_chars < total) dlg_chars = total;
                    else if (dlg[dlg_i + 1]) { dlg_i++; dlg_chars = 0; }
                    else { int a = dlg_act; dlg = 0; state = S_PLAY; do_act(a); }
                }
                continue;
            }
            /* S_PLAY */
            int d = -1;
            if (k == MEOS_KEY_UP || k == 'w' || k == 'W') d = UP;
            else if (k == MEOS_KEY_DOWN || k == 's' || k == 'S') d = DOWN;
            else if (k == MEOS_KEY_LEFT || k == 'a' || k == 'A') d = LEFT;
            else if (k == MEOS_KEY_RIGHT || k == 'd' || k == 'D') d = RIGHT;
            if (d >= 0) { hold_dir = d; face = d; hold_t = 12; }
            else if (space || enter || k == 'e' || k == 'j') { if (!try_talk()) swing_start(); }
            else if (k == '1') { }
            else if (k == '2') use_bridge();
            else if (k == '3') use_shield();
            else if (k == 'h' || k == 'H') {
                static const char *const *H[] = { D_hint0, D_hint1, D_hint2, D_hint3, D_hint4, D_hint5 };
                say(H[prog > 5 ? 5 : prog], A_NONE);
            }
            if (state != S_PLAY) break;
        }
        if (quit) { meos_display_close(); return; }

        switch (state) {
        case S_TITLE: draw_title(); break;
        case S_PLAY: update_play(); if (state == S_PLAY || state == S_DIALOG || state == S_DEAD) draw_world(); if (state == S_DIALOG) draw_dialog(); break;
        case S_DIALOG:
            if (dlg && dlg_chars < page_chars(dlg[dlg_i]) && (tick & 1)) dlg_chars += 2;
            draw_world(); draw_dialog();
            break;
        case S_DEAD:
            draw_world();
            center(250, "GAME OVER", C(255, 255, 255), C(120, 0, 20), 8);
            center(340, "探偵は、まだ倒れない…", C(255, 255, 255), C(120, 0, 20), 3);
            if ((tick >> 4) & 1) center(420, "PRESS ENTER", C(255, 230, 80), C(120, 0, 20), 3);
            break;
        case S_CLEAR:
            rect(0, 0, SW, SH, C(10, 8, 30));
            for (int i = 0; i < 60; i++) { int h = hash(i, tick / 20, 2); rect((h * 7) % SW, (h * 13) % SH, 3, 3, C(220, 220, 255)); }
            center(180, "THE END", C(255, 255, 255), C(10, 8, 30), 8);
            center(290, "五反田三郎の事件簿 第一話「ヤシキ猫を探せ」 完", C(255, 230, 80), C(10, 8, 30), 3);
            draw_cat(600, 400);
            if ((tick >> 4) & 1) center(540, "PRESS ENTER", C(200, 200, 230), C(10, 8, 30), 3);
            break;
        }
        frame_wait(&last, 33);
    }
}
