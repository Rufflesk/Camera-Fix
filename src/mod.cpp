#include "mods/service.hpp"
#include "mods/svc/hook.hpp"
#include "mods/svc/log.hpp"
#include "mods/svc/log.h"
#include "d/actor/d_a_alink.h"
#include "d/d_camera.h"
#include "d/d_com_inf_game.h"
#include <cstdio>

DEFINE_MOD();

IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(HookService, svc_hook);

static int s_patchedStyle = -1;
static f32 s_savedRate[2] = {0.0f, 0.0f};

static void restore_camera_params() {
    if (s_patchedStyle < 0) return;
    daAlink_c* link = daAlink_getAlinkActorClass();
    camera_process_class* cam = dComIfGp_getCamera(0);
    if (link != nullptr && cam != nullptr) {
        dCamParam_c& prm = cam->mCamera.mCamParam;
        if (prm.mCurrentStyle != nullptr && prm.mStyleID == s_patchedStyle) {
            prm.SetVal(s_patchedStyle, 22, s_savedRate[0]);
            prm.SetVal(s_patchedStyle, 24, s_savedRate[1]);
        }
    }
    s_patchedStyle = -1;
}

static void update_camera_fix() {
    daAlink_c* link = daAlink_getAlinkActorClass();
    camera_process_class* cam = dComIfGp_getCamera(0);
    if (link == nullptr || cam == nullptr) {
        s_patchedStyle = -1;
        return;
    }

    dCamParam_c& prm = cam->mCamera.mCamParam;
    if (prm.mCurrentStyle == nullptr) {
        s_patchedStyle = -1;
        return;
    }

    const int style = cam->mCamera.mCamStyle;
    const bool onFoot = prm.Algorythmn() == 1;

    const f32 sx = cam->mCamera.mPadInfo.mCStick.mLastPosX;
    const f32 sy = cam->mCamera.mPadInfo.mCStick.mLastPosY;
    const bool stickActive = sx > 0.15f || sx < -0.15f || sy > 0.15f || sy < -0.15f;

    if (!onFoot || stickActive) {
        restore_camera_params();
        return;
    }

    if (s_patchedStyle != style) {
        s_savedRate[0] = prm.Val(style, 22);
        s_savedRate[1] = prm.Val(style, 24);
        s_patchedStyle = style;
        svc_log->info(mod_ctx, "camera fix: recentramento desligado");
    }
    prm.SetVal(style, 22, 0.0f);
    prm.SetVal(style, 24, 0.0f);
}

// ---- sonda de colisão (só lê, não altera nada) ----
static float s_prevWallUp = -12345.0f;
static int s_prevRecover = -1;
static int s_prevBumpCase = -1;
static unsigned s_prevFlags = 0xFFFFFFFFu;
static int s_probeCooldown = 0;

static void update_bump_probe() {
    daAlink_c* link = daAlink_getAlinkActorClass();
    camera_process_class* cam = dComIfGp_getCamera(0);
    if (link == nullptr || cam == nullptr) return;
    dCamera_c& c = cam->mCamera;
    if (c.mCamParam.mCurrentStyle == nullptr) return;

    if (s_probeCooldown > 0) s_probeCooldown--;

    const float wallUp = static_cast<float>(c.mWallUpDist);
    const int recover = static_cast<int>(c.mWallRecoverStepCount);
    const int bumpCase = static_cast<int>(c.mLastBumpCase);
    const unsigned flags = static_cast<unsigned>(c.mBumpCheckFlags);

    const bool changed = wallUp != s_prevWallUp || recover != s_prevRecover ||
                         bumpCase != s_prevBumpCase || flags != s_prevFlags;
    if (changed && s_probeCooldown == 0) {
        char buf[160];
        std::snprintf(buf, sizeof(buf),
                      "bump: wallUp=%f recover=%d case=%d flags=%u",
                      static_cast<double>(wallUp), recover, bumpCase, flags);
        svc_log->info(mod_ctx, buf);
        s_prevWallUp = wallUp;
        s_prevRecover = recover;
        s_prevBumpCase = bumpCase;
        s_prevFlags = flags;
        s_probeCooldown = 6;
    }
}

extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError*) {
    mods::log::info("Camera Mouse Fix v4 inicializado");
    return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
    update_camera_fix();
    update_bump_probe();
    return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
    restore_camera_params();
    return MOD_OK;
}
}