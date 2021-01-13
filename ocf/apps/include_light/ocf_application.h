#ifndef OCF_APPLICATION
#define OCF_APPLICATION

// Functions defined in the auto-generated Device Builder file
void initialize_variables(void);
int  app_init(void);
void register_resources(void);
void factory_presets_cb(size_t device, void *data);
void handle_ocf_light_server(int argc, char *argv[]);

#endif //OCF_APPLICATION
