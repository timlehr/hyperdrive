_setup_complete = False

if not _setup_complete:
    import pymel.core as pm
    import hyperdrive
    pm.evalDeferred(hyperdrive.utils.mayautil.add_hyperdrive_menu)
    _setup_complete = True