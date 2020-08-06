#ifndef OCF_APPLICATION
#define OCF_APPLICATION

const char *APP_NAME = "Temperature Server";

// Functions defined in the auto-generated Device Builder file
void initialize_variables(void);
int  app_init(void);
void register_resources(void);
void factory_presets_cb(size_t device, void *data);

#endif //OCF_APPLICATION