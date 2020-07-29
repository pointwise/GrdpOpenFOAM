## Building the OpenFOAM Grid Import Plugin

To build the **OpenFOAM** grid import plugin you must integrate the source code from 
this repository into your local PluginSDK installation.

This plugin was created with the `mkplugin` options `-c` and `-grdp`.

This plugin uses the following custom source files.
 * `PluginSDK\src\plugins\GrdpMyOpenFOAM\FaceListFile.h`
 * `PluginSDK\src\plugins\GrdpMyOpenFOAM\FoamFile.h`
 * `PluginSDK\src\plugins\GrdpMyOpenFOAM\LabelListFile.h`
 * `PluginSDK\src\plugins\GrdpMyOpenFOAM\VectorFieldFile.h`

See [How To Integrate Plugin Code][HowTo] for details.

[HowTo]: https://github.com/pointwise/How-To-Integrate-Plugin-Code
