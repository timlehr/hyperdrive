
# -----------------------------------------------------------------------------
# This source file has been developed within the scope of the
# Technical Director course at Filmakademie Baden-Wuerttemberg.
# http://technicaldirector.de
#
# Written by Tim Lehr
# Copyright (c) 2019 Animationsinstitut of Filmakademie Baden-Wuerttemberg
# -----------------------------------------------------------------------------

from Qt import QtWidgets
from maya import OpenMayaUI as omui
from shiboken2 import wrapInstance
import maya.cmds as cmds


def get_maya_main_window():
    maya_main_window_ptr = omui.MQtUtil.mainWindow()
    maya_main_window = wrapInstance(long(maya_main_window_ptr), QtWidgets.QWidget)
    return maya_main_window


def get_next_available_array_element(attr):
    num_elements = attr.getNumElements()
    if num_elements == attr.numConnectedElements():
        return attr[num_elements]
    else:
        for i in range(0, num_elements):
            if not attr[i].listConnections():
                return attr[i]


def load_hyperdrive_plugin():
    plugin_name = "hyperdrive"
    if not cmds.pluginInfo(plugin_name, q=True, l=True):
        cmds.loadPlugin(plugin_name)
    cmds.evaluator(enable=True, name='hdEvaluator')


def add_hyperdrive_menu():
    cmds.menu("HyperdriveMenu", l="Hyperdrive", to=1, p='MayaWindow')
    cmds.menuItem("HdManager",
                  label="Manager",
                  parent="HyperdriveMenu",
                  command="import hyperdrive; hyperdrive.show_manager()")