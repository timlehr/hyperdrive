# Hyperdrive
***Pose based rig caching for Maya***

![Hyperdrive Header](http://timlehr.com/wp-content/uploads/2019/04/hd-header-mockup.png)

---

:hammer: **Experimental: This is a R&D project and not yet production ready**

## Overview

Hyperdrive is a Pose-based character rig caching solution for Autodesk Maya 2017 / 2018.
 
It's heavily inspired by the pose-based rig caching solution developed and presented by Disney Animation at Siggraph 2015: ["Achieving Real-Time Playback with Production Rigs"](https://dl.acm.org/citation.cfm?id=2792519)

Unlike common caching approaches available in DCC applications, this approach doesn't rely on time-based geometry caching. Instead it is utulizing the character rig animation values to calculate a unique pose ID, which points to a certain set of deformed character meshes stored in the cache. These poses are frame independent and can be re-used across different frame-ranges and even animation scenes.

## Build / Installation

### Requirements

- CentOS 7 / Fedora 26+ (Windows currently untested)
- Autodesk Maya 2017 / 2018 with SDK installed
- [Git](https://git-scm.com)
- [CMake](https://cmake.org) - Version 3.9.1 or newer

### Build

Hyperdrive comes with a CMake script that makes building the plugin fairly simple.

I recommend using CMake GUI if you are not familiar with the CMake command line interface.

#### Using CMake GUI

1. Clone the repository using Git to a directory of your choice.
2. Open CMake GUI and choose the cloned directory as your source code root.
3. Press _Configure_. The output widget should show you which Maya SDK versions were found.
4. Press _Generate_. This will generate the make script.
5. Navigate to the repository root in your favorite shell and execute the make install command to build the plugin. 
Make sure to change the _-j_ argument to the available CPU core count on your machine.:

    `make install -j8`


#### Using CMake command line

1. Clone the repository using Git to a directory of your choice.
2. Run the CMake command to generate the make script. The console output will show, which Maya SDK versions were found.

    `cmake /path/to/hyperdrive/repo`

3. Execute the make install command to build the plugin. 
Make sure to change the _-j_ argument to the available CPU core count on your machine.:

    `make install -j8`

### Installation

After successfully building the plugin, all you need to do is to point Maya to the Hyperdrive module.

`export MAYA_MODULE_PATH=/path/to/hyperdrive/repo:&`

## Usage

Launch Maya. After Maya has finished loading, you should see a _Hyperdrive_ menu in the main window. If you are missing this menu for some reason, please make sure that the plugin is available to Maya and loaded. You can do this via _Windows - Setting/Preferences - Plug-in Manager_.

To launch the Hyperdrive GUI, click _Hyperdrive - Manager_ in the main menu. This will open the Manger UI, which is your central hub to use Hyperdrive.

### Rig Setup

1. Open _Hyperdrive Manager_ and click _Add Rig_. Enter a unique _Rig Tag_ for your setup. Ideally this is the character name _AND_ a version number. You need to update your Rig Tag everytime your Rig changes.
2. Select your character meshes and click _Add Mesh_ in the _Caches_ Tab. This will create a _HdCacheNode_ for each mesh and connect them to your _HdPoseNode_.
3. Select your animation controls and click _Add Control Attrs._ in Controls. Hyperdrive connects your animation controls to the pose node of the rig and you are good to go.
4. _Optional_: Use the _Blacklist_ / _Whitelist_ tabs to add nodes to be explicitly evaluated all the time / never.

To temporarily bypass the cache after the setup, go to _Settings_ and check _Bypass_.

## Build Developer Documentation

### Required Python packages

- [Sphinx](https://pypi.org/project/Sphinx/) 
- [RTD Sphinx Theme](https://pypi.org/project/sphinx-rtd-theme/)
- [exhale](https://pypi.org/project/exhale/)
- [recommonmark](https://pypi.org/project/recommonmark/)

### Sphinx Docs Build

1. Install the necessary Python packages.

    ```
    $ pip install sphinx sphinx-rtd-theme exhale recommonmark
    ```

2. Navigate to ``path\to\hyperdrive\repository\docs`` and execute the make command.

    ```
    $ make html
    ```

3. The built documentation is in ``path\to\hyperdrive\repository\docs\_build\html``.

## Known Issues & Limitations

- Meshes sometimes lose material on scrubbing.
- For some complex rigs, animation edits of already cached regions might lead to inaccurate re-caching.
- Sometimes pose node to cache node connections are not being stored properly (in file), even though they appear correctly in the node editor.
- Sometimes Cache setups created via GUI do not trigger Maya to update the cacheId. The cache node needs to be disconnected and reconnected to fix this.
- The evaluator sometimes fails to update it's internal state when the scene changes.
- Meshes attached via follicle or constraints may result in incorrect cache geometry.
- M3dView currently breaks build on Linux so support for status text in viewport is not possible.
- The current prototype works best when being used with a bunch of smaller meshes instead of one big mesh (this is a Maya drawing limitation).
- Using a lot of cached meshes results in less and less performance gain.


## Possible future extensions

- Whitelisting / Blacklisting nodetype instead of specific nodes
- Log to file
- Cache to file 
- Draw caches with OpenGL directly
- Draw Hyperdrive state text in viewport

## References
- ["Achieving Real-Time Playback with Production Rigs"](https://dl.acm.org/citation.cfm?id=2792519)
- [Parallel Processing within a Host Application](http://www.multithreadingandvfx.org/course_notes/2017/disney_MultiThreading_SIGGRAPH2017.pdf)
- [Using Parallel Maya 2018](http://download.autodesk.com/us/company/files/2018/UsingParallelMaya.pdf)
- [Siggraph 2017: Parallel Evluation of Animated Maya Characters](http://www.multithreadingandvfx.org/course_notes/2017/ParallelMaya_SIGGRAPH_2017.pdf)
- [Around The Corner - Maya Development Blog](https://around-the-corner.typepad.com)

## Attributions

This project is relying on some awesome open source code to work.

- [spdlog](https://github.com/gabime/spdlog)
- [lrucache11](https://github.com/mohaps/lrucache11)
- [Qt.py](https://github.com/mottosso/Qt.py)

## License

```
MIT License

Copyright (c) 2019 Animationsinstitut of Filmakademie Baden-Wuerttemberg

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
