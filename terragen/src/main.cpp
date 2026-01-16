#include "TerragenApp.h"

int main(int argc, char** argv) {
    auto app = Genesis::CreateApplication(argc, argv);
    app->Run();
    delete app;
    return 0;
}
