#include <godot_cpp/godot.hpp>
#include <godot_cpp/core/class_db.hpp>
#include "wrapper.cpp"

using namespace godot;

void init_ws_lib(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) return;

    ClassDB::register_class<ZappyWS>();
}

void remove_ws_lib(ModuleInitializationLevel p_level) {}

extern "C"
{
    GDExtensionBool GDE_EXPORT ws_lib_init(GDExtensionInterfaceGetProcAddress p_get_proc_address,
                                                GDExtensionClassLibraryPtr p_library,
                                                GDExtensionInitialization *r_initialization)
    {
        godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(init_ws_lib);
        init_obj.register_terminator(remove_ws_lib);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}
