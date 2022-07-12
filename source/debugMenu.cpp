#include "debugMenu.hpp"
#include "timeWarp.h"
// #include "fl/server.h"
// #include "ips.hpp"

static const char* DBG_FONT_PATH = "DebugData/Font/nvn_font_jis1.ntx";
static const char* DBG_SHADER_PATH = "DebugData/Font/nvn_font_shader_jis1.bin";
static const char* DBG_TBL_PATH = "DebugData/Font/nvn_font_jis1_tbl.bin";

sead::TextWriter* gTextWriter;

void setupDebugMenu(GameSystem* gSys)
{
    // gLogger->LOG("Preparing Debug Menu.\n");

    sead::Heap* curHeap = al::getCurrentHeap();

    agl::DrawContext* context = gSys->mSystemInfo->mDrawInfo->mDrawContext;

    if (curHeap) {
        if (context) {
            // gLogger->LOG("Creating Debug Font Instance.\n");

            sead::DebugFontMgrJis1Nvn::sInstance = sead::DebugFontMgrJis1Nvn::createInstance(curHeap);

            // gLogger->LOG("Checking if Paths Exist: {%s} {%s} {%s}\n", DBG_FONT_PATH, DBG_SHADER_PATH, DBG_TBL_PATH);

            if (al::isExistFile(DBG_FONT_PATH) && al::isExistFile(DBG_SHADER_PATH) && al::isExistFile(DBG_TBL_PATH)) {

                // gLogger->LOG("Found All Files in Debug Directory!\n");

                sead::DebugFontMgrJis1Nvn::sInstance->initialize(curHeap, DBG_SHADER_PATH, DBG_FONT_PATH, DBG_TBL_PATH, 0x100000);

                // gLogger->LOG("Initialized Instance!\n");

                sead::TextWriter::setDefaultFont(sead::DebugFontMgrJis1Nvn::sInstance);

                // gLogger->LOG("Set Default Text Writer Font!\n");

                gTextWriter = new sead::TextWriter(context);

                // gLogger->LOG("Created Text Writer!\n");

                gTextWriter->setupGraphics(context);

                // gLogger->LOG("Setup Graphics!\n");
            }
        } else {
            // gLogger->LOG("Failed to get Context!\n");
        }
    } else {
        // gLogger->LOG("Failed to get Heap!\n");
    }

    TimeContainer& container = getTimeContainer();
    container.init();

    __asm("MOV W23, #0x3F800000");
    __asm("MOV W8, #0xFFFFFFFF");
}

void drawBackground(agl::DrawContext* context)
{
    sead::Vector3<float> p1r(1, .3, 0); // top left
    sead::Vector3<float> p2r(.2, .3, 0); // top right
    sead::Vector3<float> p3r(1, -1, 0); // bottom left
    sead::Vector3<float> p4r(.2, -1, 0); // bottom right

    sead::Color4f c(.1, .1, .1, .9);

    agl::utl::DevTools::beginDrawImm(context, sead::Matrix34<float>::ident, sead::Matrix44<float>::ident);

    // Right debug menu
    agl::utl::DevTools::drawTriangleImm(context, p1r, p2r, p3r, c);
    agl::utl::DevTools::drawTriangleImm(context, p3r, p4r, p2r, c);
}