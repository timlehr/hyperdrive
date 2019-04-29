
# -----------------------------------------------------------------------------
# This source file has been developed within the scope of the
# Technical Director course at Filmakademie Baden-Wuerttemberg.
# http://technicaldirector.de
#
# Written by Tim Lehr
# Copyright (c) 2019 Animationsinstitut of Filmakademie Baden-Wuerttemberg
# -----------------------------------------------------------------------------

import json
import pymel.core as pm
from .utils import logger
log = logger.get_logger(__name__)

mem_size_factor = float(1) / float(1024)


def get_cache_list():
    encoded = pm.other.hdStats("-json")
    return json.loads(encoded)


def get_cache_dict(cache_id):
    cache_list = get_cache_list()
    for cache_dict in cache_list:
        if cache_dict["id"] == cache_id:
            return cache_dict


class HdCache(object):
    """Hyperdrive Cache class.
    This represents a cache object, not node, in Hyperdrive.
    It can be used to set the maximum cache size, query the current memory usage or clear the cache.
    """

    def __init__(self, cache_id):
        self._cache_id = cache_id

    def __repr__(self):
        return "<HdCache '{}'>".format(self.cache_id)

    @staticmethod
    def _convert_size(kb_value):
        return kb_value * mem_size_factor

    def _execute_cmd(self, cmd, *args):
        if cmd == "stats":
            pm.other.hdStats(*args)
        elif cmd == "cache":
            pm.other.hdCache(self.cache_id, *args)

    @property
    def cache_dict(self):
        return get_cache_dict(self._cache_id)

    @property
    def cache_id(self):
        return self._cache_id

    @property
    def current_mem_size(self):
        return self._convert_size(self.cache_dict["current_mem_size"])

    @property
    def pose_mem_size(self):
        return self._convert_size(self.cache_dict["item_mem_size"])

    @property
    def max_mem_size(self):
        return self._convert_size(self.cache_dict["max_mem_size"])

    @max_mem_size.setter
    def max_mem_size(self, value):
        conv_value = float(value) / float(mem_size_factor)
        self._execute_cmd("cache", "-setMaxMemSize", conv_value)
        log.info("Set maximum size for cache: '{}'. Max Size: {}mb.".format(self.cache_id, conv_value))

    @property
    def pose_count(self):
        return self._convert_size(self.cache_dict["size"])

    @property
    def max_pose_count(self):
        return self._convert_size(self.cache_dict["max_size"])

    @classmethod
    def get_all(cls):
        return [cls(x["id"]) for x in get_cache_list()]

    def clear(self):
        self._execute_cmd("cache", "-clear")
        log.info("Cleared cache: '{}'".format(self.cache_id))

    def exists(self):
        for x in get_cache_list():
            if x["id"] == self.cache_id:
                return True
        return False
