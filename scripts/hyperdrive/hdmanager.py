
# -----------------------------------------------------------------------------
# This source file has been developed within the scope of the
# Technical Director course at Filmakademie Baden-Wuerttemberg.
# http://technicaldirector.de
#
# Written by Tim Lehr
# Copyright (c) 2019 Animationsinstitut of Filmakademie Baden-Wuerttemberg
# -----------------------------------------------------------------------------

from .widgets import HdManagerWidget

from .utils import mayautil, logger
log = logger.get_logger(__name__)

_manager_window = None

def show_manager():
    global _manager_window
    log.debug("Show Hyperdrive Manager")
    if not _manager_window:
        maya_window = mayautil.get_maya_main_window()
        _manager_window = HdManagerWidget(maya_window)
    _manager_window.show(dockable=True)
