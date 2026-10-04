#include "mods/service.hpp"
#include "mods/svc/hook.hpp"
#include "mods/svc/log.hpp"
#include "d/actor/d_a_alink.h"

// Game includes
#include "d/d_item_data.h"
#include "f_op/f_op_actor_mng.h"

DEFINE_MOD();

IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(HookService, svc_hook);


// Example game hook: turn heart drops into green rupees.

DEFINE_HOOK(&daAlink_c::execute, LinkExecute);

static int g_frames = 0;

void on_link_execute_post(ModContext*, void* args, void* retval, void*) {
    g_frames++;
    if (g_frames % 300 == 0) {   // cerca de 5 segundos
        mods::log::info("hook do Link funcionando");   // <-- aqui
    }
}

static HookAction on_create_item_pre(ModContext*, void* args, void*, void*) {
    int& itemNo = mods::arg_ref<int>(args, 1);
    if (itemNo == dItemNo_HEART_e) {
        itemNo = dItemNo_GREEN_RUPEE_e;
    }
    return HOOK_CONTINUE;
}

extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError*) {
    // Installs a pre hook on fopAcM_createItem.
    // Instala o post hook em daAlink_c::execute  <-- novo
    ModResult postResult = mods::hook::add_post<LinkExecute>(on_link_execute_post);
    if (postResult != MOD_OK) {
        mods::log::error("failed to install post hook on_link_execute_post");
        return postResult;
    }

    mods::log::info("Camera Mouse Fix initialized");
    return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
    return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
    return MOD_OK;
}
}