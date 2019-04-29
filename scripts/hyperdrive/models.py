
# -----------------------------------------------------------------------------
# This source file has been developed within the scope of the
# Technical Director course at Filmakademie Baden-Wuerttemberg.
# http://technicaldirector.de
#
# Written by Tim Lehr
# Copyright (c) 2019 Animationsinstitut of Filmakademie Baden-Wuerttemberg
# -----------------------------------------------------------------------------

from Qt import QtWidgets, QtCore
import pymel.core as pm

from .utils import logger
log = logger.get_logger(__name__)


class HdCacheTableModel(QtCore.QAbstractTableModel):

    def __init__(self, pose_node, column_mapping):
        super(HdCacheTableModel, self).__init__()
        self._column_mapping = column_mapping
        self._pose_node = pose_node
        self._cache_nodes = list()
        self.refresh()

    def _get_cache_node(self, index):
        if not index.isValid():
            return None
        if not index.row() < self.rowCount():
            return None
        return self._cache_nodes[index.row()]

    def refresh(self):
        self.beginResetModel()
        log.debug("Refresh table model.")
        self._cache_nodes = self._pose_node.cache_nodes
        self.endResetModel()

    def parent(self, index):
        return QtCore.QModelIndex()

    def rowCount(self, parent=QtCore.QModelIndex()):
        return len(self._cache_nodes)

    def columnCount(self, parent=QtCore.QModelIndex()):
        return len(self._column_mapping)

    def flags(self, index):
        flags = super(HdCacheTableModel, self).flags(index)
        return flags | QtCore.Qt.ItemIsEditable

        cache_node = self._get_cache_node(index)
        if cache_node and cache_node.cache.exists():
            if "edit_attr" in self._column_mapping[index.row()]:
                return flags | QtCore.Qt.ItemIsEditable
        return flags

    def data(self, index, role=QtCore.Qt.UserRole):
        cache_node = self._get_cache_node(index)
        if not cache_node:
            return None

        if role == QtCore.Qt.DisplayRole:
            return getattr(cache_node, self._column_mapping[index.column()]["display_attr"])
        elif role == QtCore.Qt.EditRole:
            return getattr(cache_node, self._column_mapping[index.column()]["edit_attr"])
        elif role == QtCore.Qt.UserRole:
            return cache_node

    def setData(self, index, value, role=QtCore.Qt.EditRole):
        cache = self._get_cache_node(index)
        if not cache:
            return

        if role == QtCore.Qt.EditRole:
            setattr(cache, self._column_mapping[index.column()]["edit_attr"], value)

    def headerData(self, section, orientation, role=QtCore.Qt.DisplayRole):
        if orientation == QtCore.Qt.Horizontal and role == QtCore.Qt.DisplayRole:
            return self._column_mapping[section]["header_str"]
        return None


class MayaNodeTableModel(QtCore.QAbstractTableModel):

    def __init__(self, pose_node, list_attr, column_mapping):
        super(MayaNodeTableModel, self).__init__()
        self._list_attr = list_attr
        self._column_mapping = column_mapping
        self._pose_node = pose_node
        self._nodes = list()
        self.refresh()

    def _get_node(self, index):
        if not index.isValid():
            return None
        if not index.row() < self.rowCount():
            return None
        return self._nodes[index.row()]

    def refresh(self):
        self.beginResetModel()
        log.debug("Refresh nodes model.")
        result = getattr(self._pose_node, self._list_attr)
        self._nodes = list(result() if callable(result) else result)
        self.endResetModel()

    def parent(self, index):
        return QtCore.QModelIndex()

    def rowCount(self, parent=QtCore.QModelIndex()):
        return len(self._nodes)

    def columnCount(self, parent=QtCore.QModelIndex()):
        return len(self._column_mapping)

    def data(self, index, role=QtCore.Qt.UserRole):
        node = self._get_node(index)
        if not node:
            return None

        if role == QtCore.Qt.DisplayRole:
            result = getattr(node, self._column_mapping[index.column()]["display_attr"])
            if callable(result):
                return result()
            return result
        elif role == QtCore.Qt.EditRole:
            return getattr(node, self._column_mapping[index.column()]["edit_attr"])
        elif role == QtCore.Qt.UserRole:
            return node

    def setData(self, index, value, role=QtCore.Qt.EditRole):
        node = self._get_node(index)
        if not node:
            return
        if role == QtCore.Qt.EditRole:
            setattr(node, self._column_mapping[index.column()]["edit_attr"], value)

    def headerData(self, section, orientation, role=QtCore.Qt.DisplayRole):
        if orientation == QtCore.Qt.Horizontal and role == QtCore.Qt.DisplayRole:
            return self._column_mapping[section]["header_str"]
        return None
