#include <stdio.h>
#include <stdlib.h>
#include "pluginmanager.h"

static void init(void)
{
	if (!load_plugins()) {
		printf("Error loading plugins!\n");
		exit(1);
	}
	printf("%d plugins loaded:\n", get_plugin_count());
	for (int i = 0; i < get_plugin_count(); i++) {
		Plugin *p = get_plugin(i);
		PluginInfo info = p->get_info();
		printf("%s: %s\n", info.name, info.description);
	}
}

static void finish(void)
{
	free_plugins();
}

int main(void)
{
	init();

	finish();
	return 0;
}
