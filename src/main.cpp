#include "TriangleApp.hpp"

#include <iostream>
#include <stdexcept>
#include <cstdlib>

int main() {
    try
    {
        TriangleApp app;
        app.run();
    }

    catch (const std::exception e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}