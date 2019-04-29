
# -----------------------------------------------------------------------------
# This source file has been developed within the scope of the
# Technical Director course at Filmakademie Baden-Wuerttemberg.
# http://technicaldirector.de
#
# Written by Tim Lehr
# Copyright (c) 2019 Animationsinstitut of Filmakademie Baden-Wuerttemberg
# -----------------------------------------------------------------------------

import pymel.core as pm

from . import utils
from .hdcache import HdCache
from .utils import logger
log = logger.get_logger(__name__)


class HdPoseNode(object):
    """Hyperdrive pose node class.
    This represents a rig setup in terms of logic.
    The wrapper can be used to setup the rig with the pose node or black- / whitelist nodes.
    """

    _native_node_type = "hyperdrivePose"

    def __init__(self, native_node):
        self._native_node = native_node

    def __repr__(self):
        return "<HdPoseNode '{}'>".format(self.rig_tag)
        
    @property
    def native_node(self):
        return self._native_node

    @property
    def name(self):
        return self.native_node.name()

    @property
    def rig_tag(self):
        return self.native_node.inRigTag.get()

    @rig_tag.setter
    def rig_tag(self, value):
        self.native_node.inRigTag.set(value)

    @property
    def pose_id(self):
        return self.native_node.outPoseId.get()
    
    @property
    def cache_ids(self):
        return [x.get() for x in self.native_node.outCacheIds]

    @property
    def caches(self):
        return [HdCache(x) for x in self.cache_ids]

    @property
    def cache_nodes(self):
        return [HdCacheNode(x) for x in self.native_node.outCacheIds.outputs()]
    
    @property
    def freeze_rig(self):
        return self.native_node.outFreezeRig.get()
    
    @property
    def enabled(self):
        return not bool(self.native_node.nodeState.get())
    
    @enabled.setter
    def enabled(self, value):
        self.native_node.nodeState.set(int(not value))

    def exists(self):
        try:
            pm.PyNode(unicode(self.native_node))
            return True
        except pm.MayaObjectError:
            return False

    # MESHES

    def add_meshes(self, *mesh_nodes):
        for mesh_node in mesh_nodes:
            cache_node = HdCacheNode.create(self)
            cache_node.add_mesh(mesh_node)

    # CONTROLLER ATTRIBUTES

    def get_controller_attrs(self):
        return self.native_node.inCtrlVals.inputs(plugs=True)

    def remove_controller_attrs(self, *attrs):
        for attr in attrs:
            if attr in self.native_node.inCtrlVals.inputs(plugs=True):
                self.native_node.inCtrlVals.disconnect(attr)

    def add_controller_attrs(self, *attrs):
        for attr in attrs:
            try:
                attr.connect(self.native_node.inCtrlVals, nextAvailable=True)
            except RuntimeError:
                log.warning("Error connecting '{}'.".format(attr))

    def add_keyable_controller_attrs(self, ctrl_node):
        self.add_controller_attrs(*[x for x in ctrl_node.listAttrs() if x.isKeyable()])

    # WHITELIST

    def get_whitelisted_nodes(self):
        return self.native_node.inWhitelist.inputs()

    def remove_whitelisted_nodes(self, *nodes):
        for node in nodes:
            if node in self.native_node.inWhitelist.inputs():
                self.native_node.inWhitelist.disconnect(node)

    def add_whitelisted_nodes(self, *nodes):
        for node in nodes:
            try:
                whitelist_attr = utils.mayautil.get_next_available_array_element(self.native_node.inWhitelist)
                node.message.connect(whitelist_attr)
                log.debug("Whitelisted node: '{}'".format(node))
            except RuntimeError:
                log.warning("Error whitelisting '{}'.".format(node.name()))

    # BLACKLIST

    def get_blacklisted_nodes(self):
        return self.native_node.outFreezeRig.outputs()

    def remove_blacklisted_node(self, *nodes):
        for node in nodes:
            if node in self.native_node.outFreezeRig.outputs():
                self.native_node.outFreezeRig.disconnect(node)

    def add_blacklisted_nodes(self, *nodes):
        for node in nodes:
            if node.frozen.isConnected():
                log.warning("Node '{}' already has a attribute connected to frozen. We will rewire it.".format(node))
                node.frozen.disconnect()
            try:
                self.native_node.outFreezeRig.connect(node.frozen)
                log.debug("Blacklisted node: '{}'".format(node))
            except RuntimeError:
                log.warning("Error blacklisting '{}'.".format(node.name()))

    def update_blacklist_from_input_meshes(self):
        for cache_node in self.cache_nodes:
            self.add_blacklisted_nodes(*cache_node.get_in_mesh_nodes())

    def update_blacklist_from_nodetypes(self,  *nodetypes, **kwargs):
        for node in pm.Namespace(kwargs.get("namespace", ":")).listNodes(recursive=True):
            if node.nodeType() in nodetypes:
                self.add_blacklisted_nodes(node)

    @classmethod
    def create(cls, rig_tag, **kwargs):
        native_node = pm.createNode(cls._native_node_type, ss=True)
        pose_node = cls(native_node)
        pose_node.rig_tag = rig_tag
        return pose_node

    @classmethod
    def get_all(cls):
        return [cls(node) for node in pm.ls(exactType=cls._native_node_type)]


class HdCacheNode(object):

    _native_node_type = "hyperdriveCache"

    def __init__(self, cache_node):
        self._native_node = cache_node

    def __repr__(self):
        return "<HdCacheNode '{}'>".format(self.cache_id)

    @property
    def name(self):
        return self.native_node.name()

    @property
    def pose_node(self):
        if self.native_node.inPoseId.isConnected():
            return HdPoseNode(self.native_node.inPoseId.inputs()[0])
        return

    @property
    def cache_id(self):
        return self.native_node.inCacheId.get()

    @property
    def cache(self):
        return HdCache(self.cache_id)

    @property
    def native_node(self):
        return self._native_node

    @property
    def display_name(self):
        mesh_node_str = ", ".join([x.name() for x in self.get_out_mesh_nodes()])
        return "{} ({})".format(self.name, mesh_node_str)

    @property
    def display_mem_size(self):
        cache = self.cache
        if not cache.exists():
            return "Uninitialized"
        percent = "({:.0f}%)".format((float(cache.current_mem_size) / float(cache.max_mem_size)) * 100)
        return "{0:.1f} / {1:.0f} MB {2}".format(cache.current_mem_size, cache.max_mem_size, percent)

    @property
    def max_cache_mem_size(self):
        if self.cache.exists():
            return self.cache.max_mem_size
        else:
            log.warning("Cache '{}' hasn't been initialized yet.".format(self.cache.cache_id))

    @max_cache_mem_size.setter
    def max_cache_mem_size(self, value):
        if self.cache.exists():
            self.cache.max_mem_size = value
        else:
            log.warning("Cache '{}' hasn't been initialized yet.".format(self.cache.cache_id))

    def exists(self):
        try:
            pm.PyNode(unicode(self.native_node))
            return True
        except pm.MayaObjectError:
            return False

    def get_mesh_plugs(self):
        plug_tuple_list = []
        in_meshes = self.native_node.inMeshes.inputs(plugs=True)
        out_meshes = self.native_node.outMeshes.outputs(plugs=True)
        size = len(in_meshes) if len(in_meshes) >= len(out_meshes) else len(out_meshes)

        for i in xrange(0, size):
            try:
                in_mesh = in_meshes[i]
            except IndexError:
                in_mesh = None
            try:
                out_mesh = out_meshes[i]
            except IndexError:
                out_mesh = None
            plug_tuple_list.append((in_mesh, out_mesh))
        return plug_tuple_list

    def get_in_mesh_nodes(self):
        return self.native_node.inMeshes.inputs()

    def get_out_mesh_nodes(self):
        return self.native_node.outMeshes.outputs()

    def add_mesh(self, mesh_node):
        mesh_index = self.native_node.inMeshes.getNumElements()

        src_plug = mesh_node.inMesh.inputs(plugs=True)[0]

        # connect source mesh to cache node
        src_plug.connect(self.native_node.inMeshes[mesh_index])

        # disconnect target mesh from source and connect it to cache instead
        mesh_node.inMesh.disconnect()
        self.native_node.outMeshes[mesh_index].connect(mesh_node.inMesh)

    def remove_mesh(self, mesh_node):
        try:
            index = self.native_node.outMeshes.outputs().index(mesh_node)
            src_plug = self.native_node.inMeshes[index].connections(plugs=True)[0]

            self.native_node.inMeshes[index].disconnect()
            self.native_node.outMeshes[index].disconnect()

            src_plug.connect(mesh_node.inMesh)
        except IndexError:
            log.exception("Mesh node '{}' not connected to cache node '{}'.".format(mesh_node, self), exc_info=True)

    def delete(self):
        log.info("Delete cache node: '{}'".format(self))
        for mesh in self.get_out_mesh_nodes():
            self.remove_mesh(mesh)
        pm.delete(self.native_node)

    def clear_cache(self):
        if self.cache.exists():
            self.cache.clear()
        else:
            log.warning("Couldn't clear cache '{}'. Not initialized yet.".format(self.cache.cache_id))

    @classmethod
    def create(cls, pose_node, **kwargs):
        native_node = pm.createNode(cls._native_node_type, ss=True)
        pose_node.native_node.outPoseId.connect(native_node.inPoseId)

        index = pose_node.native_node.outCacheIds.getNumElements()
        attr = pose_node.native_node.outCacheIds[index]
        log.info("Assign cache ID: '{}'".format(attr.get())) # HACK: Has to be here for initial cacheId eval
        pose_node.native_node.outCacheIds[index].connect(native_node.inCacheId)


        return cls(native_node)

    @classmethod
    def get_all(cls):
        return [cls(node) for node in pm.ls(exactType=cls._native_node_type)]
