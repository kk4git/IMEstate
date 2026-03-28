#include <string>
#include <vector>
#include <cstdio>
#include <windows.h>
#include <dwmapi.h>

#pragma comment (lib, "gdi32.lib")
#pragma comment (lib, "user32.lib")
#pragma comment (lib, "dwmapi.lib")

// A class managing verbosity of info. Supports INFO? macros.
struct VerboseMan {
    static void setVerbose(int level) { instance() = level; }
    static bool isVerbose(int level) { return instance() >= level; }
private:
    static int& instance() { static int singleton = 0; return singleton; }
};
#define INFO1(fmt, ...) do { if (VerboseMan::isVerbose(1)) std::printf((fmt), __VA_ARGS__); } while(0)
#define INFO2(fmt, ...) do { if (VerboseMan::isVerbose(2)) std::printf((fmt), __VA_ARGS__); } while(0)
#define INFO3(fmt, ...) do { if (VerboseMan::isVerbose(3)) std::printf((fmt), __VA_ARGS__); } while(0)

// A class storing Black/White dots of a image.
// Each dot in a BYTE, 0 = Black, 0xFF = White.
// Default is all 0(Black).
class BW8 { 
    std::vector<BYTE> v_;
    int w_ = 0;
    int h_ = 0;

public:
    ~BW8() = default;
    BW8(const BW8&) = default;
    BW8(BW8&&) noexcept = default;
    BW8& operator=(const BW8&) = default;
    BW8& operator=(BW8&&) noexcept = default;

    BW8() = default;
    BW8(int w, int h) { resize(w, h); }
    void resize(int w, int h) { w_ = w; h_ = h; v_.assign(w_ * h_, 0); }

    int width() const noexcept { return w_; }
    int height() const noexcept { return h_; }

    BYTE& at(int x, int y) { return v_[static_cast<size_t>(y) * w_ + x]; }
    const BYTE& at(int x, int y) const { return v_[static_cast<size_t>(y) * w_ + x]; }

public:
    BW8(const UINT32* pRGB, int w, int h): v_(), w_(w), h_(h) {
        resize(w, h);
        for (int i = 0; i < w_ * h_; ++i) {
            UINT32 rgb = pRGB[i];
            auto r = GetRValue(rgb), g = GetGValue(rgb), b = GetBValue(rgb);
            int lum = (299 * r + 587 * g + 114 * b) / 1000;
            v_[i] = (lum > 127) ? 0xFF : 0x00;
        }
    }

    bool allBlack() const {
        for (int i = 0; i < w_ * h_; ++i)
            if (v_[i] > 127) return false;
        return true;
    }

    BW8 crop(int x0, int y0, int w = 96, int h = 96) const {
        BW8 out(w, h);
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                out.at(x, y) = at(x+x0, y+y0);
        return out;
    }

    int numB(int x0, int y0, int w, int h) {
        int cnt = 0;
        for (int y = y0; y < y0 + h; y++) {
            for (int x = x0; x < x0 + w; x ++)
                if (at(x,y) <= 0x80)
                    ++cnt;
        }
        debugMark(x0, y0, w, h);
        return cnt;
    }

public:
    void debugMark(int x0, int y0, int w, int h) { // Make 0->1, 0xFF->0xFE
        auto mark = [](BYTE& v) { if (0==v) v=1; if (0xFF==v) v=0xFE; };
        for (int y = y0; y < y0 + h; ++y) { // 2 vertical lines
            mark(at(x0, y)); mark(at(x0+w-1, y));
        }
        for (int x = x0; x < x0 + w; ++x) { // 2 horizontal lines
            mark(at(x, y0)); mark(at(x, y0+h-1));
        }
    }

    void debugDraw(int skip = 1) const { // Draw all dots to log, with debug marks.
        auto drawRule = [](int w, int skip = 1) {
            INFO3("\n");
            for (int mod: {100,10,1}) {
                std::string line;
                for (int x = 0; x < w; x += skip)
                    line += "0123456789"[(x/mod)%10];
                INFO3("%s\n", line.c_str());
            }
            INFO3("\n");
        };
        auto ascii = [](BYTE bw) { 
            switch(bw) {
                case 0x00: return ' '; // Normal black
                case 0x01: return '.'; // Marked black
                case 0xFE: return '@'; // Marked white
                case 0xFF: return '*'; // Normal white
            }
            return 'E'; // Error!
        };
        drawRule(w_, skip);
        for (int y = 0; y < h_; y += skip) {
            std::string line;
            for (int x = 0; x < w_; x += skip)
                line += ascii(at(x, y));
            char buf[20];
            snprintf(buf, 20, " %03d", y);
            line += buf;
            INFO3("%s\n", line.c_str());
        }
    }
};

static int imeFull(BW8& bw8) {
    int c1 = bw8.numB(24,30, 12,12), c2 = bw8.numB(45,72, 12, 9);
    int z1 = bw8.numB( 0, 0, 96,21), z2 = bw8.numB( 0,84, 96,12);
    int z3 = bw8.numB( 0,24, 18,57), z4 = bw8.numB(66,24, 30,57);
    int z5 = bw8.numB(21,51, 21,30), z6 = bw8.numB(45,24, 21,18);
    bw8.debugDraw(); 
    INFO2("imeFull: Check = (%d,%d), Zero = (%d,%d,%d,%d,%d,%d). ", c1, c2, z1, z2, z3, z4, z5, z6);
    if (z1 + z2 + z3 + z4 + z5 + z6 > 20) return -1; // 0,0,0,0,0,0,0
    if (c1 > 100 && c2 < 12) return 1;               // Half: 142, 7
    if (c1 < 100 && c2 > 12) return 2;               // Full:  52,29
    return -1;
}

static int imeWide(BW8& bw8) {
    int c1 = bw8.numB(48,36, 15,28), c2 = bw8.numB(21,27, 18,24), c3 = bw8.numB(30,63, 27, 9);
    int z1 = bw8.numB( 6,12, 18,12), z2 = bw8.numB(66,12, 18,12);
    int z3 = bw8.numB( 6,75, 18,12), z4 = bw8.numB(66,75, 18,12);
    bw8.debugDraw(); 
    INFO2("imeWide: Check = SUM(%d,%d,%d)<120, SUM(%d,%d,%d,%d) <= 20. ", c1, c2, c3, z1, z2, z3, z4);
    if (z1 + z2 + z3 + z4 > 20) return -1;    // 9,0,0,0
    if (c1 + c2 + c3 < 120) return 1;         // Narrow: 3,27,45
    if (c1>200 && c2>200 && c3>120) return 2; // Wide: 420,432,234
    return -1;
}

static int imeEnCn(BW8& bw8) {
    int ul = bw8.numB(27,24,  9, 9), um = bw8.numB(38,23,  8, 7), ur = bw8.numB(48,24,  9, 9);
    int dl = bw8.numB(12,72, 18,12), dm = bw8.numB(36,71, 12,13), dr = bw8.numB(54,72, 18,12);
    bw8.debugDraw();
    INFO2("imeEnCN: UP = (%d,%d,%d), DOWN = (%d,%d,%d). ", ul, um, ur, dl, dm, dr);
    if (abs(ul-ur)>10 || abs(dl-dr)>10) return -1; // Symmetry
    if (ul>30 && um<20 && ur>30 && dl>30 && dm<20 && dr>30) return 1; // En: (48,16,48) (61, 0,61)
    if (ul<30 && um>20 && ur<30 && dl<30 && dm>20 && dl<30) return 2; // Cn: ( 9,28, 9) ( 0,36, 0)
    return -1;
}

static int examIME(BW8& bw8) {
    if (bw8.allBlack()) {
        INFO1("examIME: All black, IME window invisible.\n");
        return -1;
    }

    BW8 i1, i2, i3;
    if (bw8.width() > bw8.height()) {
        int x = 78; i1 = bw8.crop(x, 6); x += 96; i2 = bw8.crop(x, 6); x += 96; i3 = bw8.crop(x, 6);
    } else {
        int y = 78; i1 = bw8.crop(6, y); y += 96; i2 = bw8.crop(6, y); y += 96; i3 = bw8.crop(6, y);
    }

    int encn = imeEnCn(i1); INFO1("imeEnCn = %d\n", encn);
    int wide = imeWide(i2); INFO1("imeWide = %d\n", wide);
    int full = imeFull(i3); INFO1("imeFull = %d\n", full);

    if (encn<0 || wide<0 || full<0) return -1;
    return encn*100 + wide*10 + full;
}

static int pixelScale3(HWND hwnd, int w, int h) { // Scale 300% * 96 = 288(DPI)
    int w3 = w>h? 552:108, h3 = w>h? 108:564;

    HDC dc0 = GetDC(0), dcMem = CreateCompatibleDC(dc0), dc3 = CreateCompatibleDC(dc0);
    HBITMAP bmMem = CreateCompatibleBitmap(dc0, w, h), bm3 = CreateCompatibleBitmap(dc0, w3, h3);
    HGDIOBJ obMem = SelectObject(dcMem, bmMem), ob3 = SelectObject(dc3, bm3);
    if (!PrintWindow(hwnd, dcMem, PW_RENDERFULLCONTENT)) {
        INFO1("pixelS3: PrintWindow failed!\n");
        if (HDC dcWin = GetWindowDC(hwnd)) {
            BitBlt(dcMem, 0, 0, w, h, dcWin, 0, 0, SRCCOPY | CAPTUREBLT);
            ReleaseDC(hwnd, dcWin);
        }
    }
    SetStretchBltMode(dc3, HALFTONE);
    StretchBlt(dc3, 0, 0, w3, h3, dcMem, 0, 0, w, h, SRCCOPY);
    SelectObject(dc3, ob3); DeleteDC(dc3);
    SelectObject(dcMem, obMem); DeleteDC(dcMem);

    BITMAPINFO bi = { 0 };
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biCompression = BI_RGB;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biWidth = w3;
    bi.bmiHeader.biHeight = -h3;

    UINT32* p32RGB = new UINT32[h3 * w3]; // Get RGB32 data
    int got = GetDIBits(dc0, bm3, 0, h3, p32RGB, &bi, DIB_RGB_COLORS);
    DeleteObject(bm3); DeleteObject(bmMem); ReleaseDC(NULL, dc0);

    int ret = -1;
    if (!got) INFO1("pixelS3: GetDIBits failed!\n");
    else {
        BW8 bw8(p32RGB, w3, h3);
        ret = examIME(bw8);
    }
    delete[] p32RGB;
    return ret;
}

static int examWin(HWND hwnd) {
    if (!IsWindow(hwnd)) {
        INFO2("examWin: invalid HWND=0x%p!\n", hwnd);
        return -1;
    }

    DWORD pid, tid = GetWindowThreadProcessId(hwnd, &pid);
    auto style = GetWindowLongPtr(hwnd, GWL_STYLE), exstyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);

    UINT dpi = GetDpiForWindow(hwnd); // NOT accurate!
    RECT rc;
    if (S_OK != DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rc, sizeof(RECT))) return -1;
    int w = rc.right-rc.left, h = rc.bottom-rc.top;
    float rwh = float(w) / h, rhw = float(h) / w;

    INFO2("examWin: HWND=0x%IX P/TID=%d/%d Style/Ex=0x%IX/0x%IX LTRB(%d,%d,%d,%d)\n",
                   (size_t)hwnd, pid, tid, style, exstyle, rc.left, rc.top, rc.right, rc.bottom);
    INFO2("\t Size(%d,%d) DPI=%d Ratio(%.4f,%.4f) ", w, h, dpi, rwh, rhw);

    if (abs(rwh-5.11111f) > 0.051111f && abs(rhw-5.22222f) > 0.052222f) {
        INFO2("\t Wrong ratio!\n");
        return -1;
    }
    INFO2("\n");

    return  pixelScale3(hwnd, w, h);
}

static int walkWin() {
    const char *cls = "ApplicationFrameWindow";
    HWND h = FindWindowExA(NULL, NULL, cls, "");
    int ret = -1;
    while (h) {
        int r = examWin(h);
        if (r > 0) ret = r;
        h = FindWindowExA(NULL, h, cls, "");
    }
    return ret;
}

static void initConsole(std::string cmd) {
    if (!cmd.length()) return;

    FILE* f; // Cmd not empty, verbose or help.
    AttachConsole(ATTACH_PARENT_PROCESS);
    freopen_s(&f, "CONIN$", "r", stdin);
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);

    int v = 0;
    if (!cmd.find("-v")) v = 1;
    if (!cmd.find("-vv")) v = 2;
    if (!cmd.find("-vvv")) v = 3;
    VerboseMan::setVerbose(v);

    std::printf(v? "Verbose level: %d\n" : "IMEstate: Retrive states of IME window.\n  -v, -vv, -vvv: Verbose level.\n", v);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    initConsole(lpCmdLine);
    int rc = walkWin();
    INFO1("Final: %d\n", rc);
    return rc;
}
