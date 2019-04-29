
# -----------------------------------------------------------------------------
# This source file has been developed within the scope of the
# Technical Director course at Filmakademie Baden-Wuerttemberg.
# http://technicaldirector.de
#
# Written by Tim Lehr
# Copyright (c) 2019 Animationsinstitut of Filmakademie Baden-Wuerttemberg
# -----------------------------------------------------------------------------

from __future__ import division

from abc import abstractmethod
from random import randint
import logging

import pymel.core as pm

from collections import OrderedDict
from Qt import QtWidgets, QtGui, QtCore
from maya.app.general.mayaMixin import MayaQWidgetDockableMixin

from . import HdPoseNode, utils
from .models import HdCacheTableModel, MayaNodeTableModel
from .utils import logger

log = logger.get_logger(__name__)


class HdManagerWidget(MayaQWidgetDockableMixin, QtWidgets.QWidget):
    """Main Hyperdrive Manager window. Dockable in Maya main window.
    """
    def __init__(self, parent, *args, **kwargs):
        super(HdManagerWidget, self).__init__(parent, *args, **kwargs)

        utils.mayautil.load_hyperdrive_plugin()

        self._cache_tab = None
        self._whitelist_tab = None
        self._blacklist_tab = None
        self._controller_widget = None
        self._settings_widget = None

        self._setup_ui()
        self._update_rig_select_cbx()

    def _setup_ui(self):
        self.setWindowTitle("Hyperdrive")
        self._tabwidget = QtWidgets.QTabWidget()
        self._layout = QtWidgets.QVBoxLayout()
        self._rig_layout = QtWidgets.QHBoxLayout()

        self._rig_select_cbx = QtWidgets.QComboBox()
        self._rig_select_cbx.currentIndexChanged.connect(self._on_rig_selection_changed)
        self._add_rig_btn = QtWidgets.QPushButton("Add Rig")
        self._add_rig_btn.clicked.connect(self._on_add_rig)

        self._rig_layout.addWidget(self._rig_select_cbx)
        self._rig_layout.addWidget(self._add_rig_btn)

        self._layout.addLayout(self._rig_layout)
        self._layout.addWidget(self._tabwidget)
        self.setLayout(self._layout)
        self.installEventFilter(self)

    def update_ui(self, pose_node=None):
        self._tabwidget.clear()
        if pose_node:
            self._cache_tab = CacheWidget(pose_node)
            self._whitelist_tab = WhitelistWidget(pose_node)
            self._blacklist_tab = BlacklistWidget(pose_node)
            self._controller_widget = CtrlAttrsWidget(pose_node)
            self._settings_widget = SettingsWidget(pose_node)

            self._tabwidget.addTab(self._cache_tab, "Caches")
            self._tabwidget.addTab(self._controller_widget, "Controls")
            self._tabwidget.addTab(self._whitelist_tab, "Whitelist")
            self._tabwidget.addTab(self._blacklist_tab, "Blacklist")
            self._tabwidget.addTab(self._settings_widget, "Settings")

    def _on_add_rig(self):
        text, accepted = QtWidgets.QInputDialog.getText(self,
                                                        "Add Rig to Hyperdrive",
                                                        "Please enter a unique ID for your rig. e.g. 'Chewie_v009'",
                                                        QtWidgets.QLineEdit.Normal, "")
        if accepted:
            if not text:
                rand_number = "{}".format(randint(0, 999999))
                rig_tag = "RigTag_{}".format(rand_number.zfill(6))
            else:
                rig_tag = text
            self._create_pose_node(rig_tag)

    def _update_rig_select_cbx(self, active_pose_node=None):

        current_pose_node = self._rig_select_cbx.currentData() if self._rig_select_cbx.count() > 0 else None

        self._rig_select_cbx.clear()

        pose_nodes = HdPoseNode.get_all()

        if not pose_nodes:
            self._rig_select_cbx.addItem("- No configured rigs -", None)
        else:
            for pose_node in pose_nodes:
                self._rig_select_cbx.addItem(pose_node.rig_tag, pose_node)

            if active_pose_node in pose_nodes:
                self._rig_select_cbx.setCurrentIndex(pose_nodes.index(active_pose_node))
            elif current_pose_node in pose_nodes:
                self._rig_select_cbx.setCurrentIndex(pose_nodes.index(current_pose_node))

    def _create_pose_node(self, rig_tag):
        node = HdPoseNode.create(rig_tag)
        self._update_rig_select_cbx(node)

    def _on_rig_selection_changed(self, index):
        pose_node = self._rig_select_cbx.itemData(index)
        self.update_ui(pose_node)

    def eventFilter(self, object, event):
        catch = [QtCore.QEvent.WindowActivate, QtCore.QEvent.FocusIn]
        if event.type() in catch:
            pose_node = self._rig_select_cbx.currentData()
            if not pose_node or not pose_node.exists():
                self._update_rig_select_cbx()
        return super(HdManagerWidget, self).eventFilter(object, event)


class CacheWidget(QtWidgets.QWidget):

    def __init__(self, pose_node):
        super(CacheWidget, self).__init__()
        self._pose_node = pose_node
        self._setup_ui()
        self.update_ui()

    def _setup_ui(self):
        mapping = [
            {"display_attr": "display_name", "header_str": "Node"},
            {"display_attr": "display_mem_size", "header_str": "Memory Size",
             "edit_attr": "max_cache_mem_size", "editor": SpinboxEditor(min_value=64, max_value=64000)},
        ]

        self._layout = QtWidgets.QVBoxLayout()
        self._layout.setContentsMargins(0, 0, 0, 0)
        self.setLayout(self._layout)

        self._node_table_view = QtWidgets.QTableView()
        self._node_table_view.setItemDelegateForColumn(1, mapping[1]["editor"])

        self._node_table_model = HdCacheTableModel(self._pose_node, mapping)
        self._node_table_view.setModel(self._node_table_model)
        self._node_table_view.horizontalHeader().setStretchLastSection(True)
        self._node_table_view.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
        self._node_table_view.verticalHeader().hide()

        self._btn_box = QtWidgets.QHBoxLayout()
        self._add_cache_node_btn = QtWidgets.QPushButton("Add Mesh")
        self._remove_cache_node_btn = QtWidgets.QPushButton("Remove Mesh")
        self._clear_cache_btn = QtWidgets.QPushButton("Clear Cache")
        self._add_cache_node_btn.clicked.connect(self._on_btn_clicked)
        self._remove_cache_node_btn.clicked.connect(self._on_btn_clicked)
        self._clear_cache_btn.clicked.connect(self._on_btn_clicked)

        self._btn_box.addWidget(self._add_cache_node_btn)
        self._btn_box.addWidget(self._remove_cache_node_btn)
        self._btn_box.addWidget(self._clear_cache_btn)

        self._layout.addWidget(self._node_table_view)
        self._layout.addLayout(self._btn_box)

        self.installEventFilter(self)
        self.setContentsMargins(0, 0, 0, 0)

    def _on_btn_clicked(self):
        sender = self.sender()
        if sender == self._add_cache_node_btn:
            log.debug("Add cache node")
            self.add_meshes()
        elif sender == self._remove_cache_node_btn:
            log.debug("Remove cache node")
            selected = self._get_selected_nodes()
            for node in selected:
                node.delete()
        elif sender == self._clear_cache_btn:
            log.debug("Clear cache")
            selected = self._get_selected_nodes()
            if not selected:
                log.info("Select at least one cache node to clear.")
            for node in selected:
                node.clear_cache()
        self.update_ui()

    def _get_selected_nodes(self):
        indexes = self._node_table_view.selectionModel().selectedRows()
        return [index.data(role=QtCore.Qt.UserRole) for index in indexes]

    def add_meshes(self):
        shapes = []
        for node in pm.selected():
            shape = node
            if shape.nodeType() == 'transform':
                shape = node.getShape()
            if shape.nodeType() == 'mesh':
                shapes.append(node)
            else:
                log.warning("'{}' is not a valid mesh node. Skip.".format(shape))
        self._pose_node.add_meshes(*shapes)

    def update_ui(self):
        self._node_table_model.refresh()

    def eventFilter(self, object, event):
        catch = [QtCore.QEvent.WindowActivate,
                 QtCore.QEvent.FocusIn]
        if event.type() in catch:
            self.update_ui()
        return super(CacheWidget, self).eventFilter(object, event)


class NodelistWidget(QtWidgets.QWidget):

    _add_btn_text = "Add"
    _remove_btn_text = "Remove"

    def __init__(self, pose_node):
        super(NodelistWidget, self).__init__()
        self._pose_node = pose_node
        self._setup_ui()
        self.update_ui()

    def _setup_ui(self):
        self._layout = QtWidgets.QVBoxLayout()
        self._layout.setContentsMargins(0, 0, 0, 0)
        self.setLayout(self._layout)

        self._node_table_view = QtWidgets.QTableView()

        self._node_table_model = self._get_table_model()
        self._node_table_view.setModel(self._node_table_model)
        self._node_table_view.horizontalHeader().setStretchLastSection(True)
        self._node_table_view.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
        self._node_table_view.verticalHeader().hide()

        self._btn_box = QtWidgets.QHBoxLayout()
        self._add_btn = QtWidgets.QPushButton(self._add_btn_text)
        self._remove_btn = QtWidgets.QPushButton(self._remove_btn_text)
        self._refresh_btn = QtWidgets.QPushButton("Refresh")

        self._add_btn.clicked.connect(self._on_btn_clicked)
        self._remove_btn.clicked.connect(self._on_btn_clicked)
        self._refresh_btn.clicked.connect(self._on_btn_clicked)

        self._btn_box.addWidget(self._add_btn)
        self._btn_box.addWidget(self._remove_btn)
        self._btn_box.addWidget(self._refresh_btn)

        self._layout.addWidget(self._node_table_view)
        self._layout.addLayout(self._btn_box)

        self.setContentsMargins(0, 0, 0, 0)

    def _on_btn_clicked(self):
        sender = self.sender()
        if sender == self._add_btn:
            log.debug("Add Node")
            self.add_nodes()
        elif sender == self._remove_btn:
            log.debug("Remove Node")
            nodes = self._get_selected_nodes()
            self.remove_nodes(nodes)
        elif sender == self._refresh_btn:
            log.debug("Refresh UI")
        self.update_ui()

    def _get_selected_nodes(self):
        return [] # TODO: Fix remove. Currently crashes for some index access reason.
        """
        indexes = self._node_table_view.selectionModel().selectedRows()
        return [index.data(role=QtCore.Qt.UserRole) for index in indexes]"""

    @abstractmethod
    def _get_table_model(self):
        raise NotImplementedError

    @abstractmethod
    def add_nodes(self):
        raise NotImplementedError

    @abstractmethod
    def remove_nodes(self, selected):
        raise NotImplementedError

    def update_ui(self):
        self._node_table_model.refresh()

    def showEvent(self, event):
        val = super(NodelistWidget, self).showEvent(event)
        self.update_ui()
        return val


class CtrlAttrsWidget(NodelistWidget):
    _add_btn_text = "Add Control Attrs."
    _remove_btn_text = "Remove Attrs."

    def _get_table_model(self):
        mapping = [
            {"display_attr": "name", "header_str": "Controller Attribute Name"}
        ]
        return MayaNodeTableModel(self._pose_node, "get_controller_attrs", mapping)

    def add_nodes(self):
        for node in pm.selected():
            t = node.listAttr(st='translate*', keyable=True)
            self._pose_node.add_controller_attrs(*t)
            s = node.listAttr(st='scale*', keyable=True)
            self._pose_node.add_controller_attrs(*s)
            r = node.listAttr(st='rotate*', keyable=True)
            self._pose_node.add_controller_attrs(*r)

    def remove_nodes(self, selected):
        self._pose_node.remove_controller_attrs(*selected)


class WhitelistWidget(NodelistWidget):
    _add_btn_text = "Add Node"
    _remove_btn_text = "Remove Node"

    def _get_table_model(self):
        mapping = [
            {"display_attr": "name", "header_str": "Node Name"}
        ]
        return MayaNodeTableModel(self._pose_node, "get_whitelisted_nodes", mapping)

    def add_nodes(self):
        self._pose_node.add_whitelisted_nodes(*pm.selected())

    def remove_nodes(self, selected):
        self._pose_node.remove_whitelisted_nodes(*selected)


class BlacklistWidget(NodelistWidget):
    _add_btn_text = "Add Node"
    _remove_btn_text = "Remove Node"

    def _get_table_model(self):
        mapping = [
            {"display_attr": "name", "header_str": "Node Name"}
        ]
        return MayaNodeTableModel(self._pose_node, "get_blacklisted_nodes", mapping)

    def add_nodes(self):
        self._pose_node.add_blacklisted_nodes(*pm.selected())

    def remove_nodes(self, selected):
        self._pose_node.remove_blacklisted_nodes(*selected)


class SettingsWidget(QtWidgets.QWidget):
    rig_tag_changed = QtCore.Signal(str)

    _log_level_mapping = OrderedDict([
        ("Debug", (3, logging.DEBUG)),
        ("Info", (2, logging.INFO)),
        ("Warning", (1, logging.WARNING)),
        ("Critical", (0, logging.CRITICAL))
    ])

    def __init__(self, pose_node):
        super(SettingsWidget, self).__init__()
        self._pose_node = pose_node
        self._setup_ui()

    def _setup_ui(self):
        self._layout = QtWidgets.QFormLayout()
        self.setLayout(self._layout)

        self._rig_tag_edit = QtWidgets.QLineEdit()
        self._log_level_cbx = QtWidgets.QComboBox()

        self._rig_tag_edit.installEventFilter(self)
        self._rig_tag_edit.setText(self._pose_node.rig_tag)
        self._log_level_cbx.addItems(self._log_level_mapping.keys())
        self._log_level_cbx.currentIndexChanged.connect(self._on_log_level_changed)

        self._hd_bypass_chx = QtWidgets.QCheckBox("Disable Hyperdrive for '{}'".format(self._pose_node.rig_tag))
        self._hd_bypass_chx.setChecked(not self._pose_node.enabled)
        self._hd_bypass_chx.stateChanged.connect(self._on_bypass_changed)

        self._layout.addRow("<b>Rig Tag:</b>", self._rig_tag_edit)
        self._layout.addRow("<b>Log Level:</b>", self._log_level_cbx)
        self._layout.addRow("<b>Bypass:</b>", self._hd_bypass_chx)

    def _on_bypass_changed(self, checked):
        self._pose_node.enabled = not checked

    def _on_log_level_changed(self, index):
        current_text = self._log_level_cbx.itemText(index)
        data = self._log_level_mapping[current_text]
        pm.other.hdLog("-verbosity", data[0])
        logger.set_log_level(data[1])

    def _on_rig_tag_edited(self):
        text = self._rig_tag_edit.text()
        if text:
            self._pose_node.rig_tag = text
            self.rig_tag_changed.emit(text)

    def eventFilter(self, object, event):
        catch = [QtCore.QEvent.KeyRelease]
        if event.type() in catch:
            self._on_rig_tag_edited()
        return super(SettingsWidget, self).eventFilter(object, event)


class SpinboxEditor(QtWidgets.QStyledItemDelegate):
    """ComboBox model instance editor."""

    def __init__(self, **kwargs):

        self._display_attr = kwargs.get("display_attr") or "display_str"
        self._none_item = kwargs.get("none_valid") if kwargs.get("none_valid") is not None else False

        self._deco_pos = kwargs.get("deco_pos", QtWidgets.QStyleOptionViewItem.Right)
        self._deco_size = kwargs.get("deco_size", QtCore.QSize(12, 12))

        self._min_value = kwargs.get("min_value")
        self._max_value = kwargs.get("max_value")

        super(SpinboxEditor, self).__init__()

    def initStyleOption(self, option, index):
        super(SpinboxEditor, self).initStyleOption(option, index)
        if self._deco_pos:
            option.decorationPosition = self._deco_pos
        if self._deco_size:
            option.decorationSize = self._deco_size

    def createEditor(self, parent, option, index):
        widget = QtWidgets.QSpinBox(parent)

        if self._min_value:
            widget.setMinimum(self._min_value)

        if self._max_value:
            widget.setMaximum(self._max_value)

        return widget

    def setEditorData(self, editor, index):
        edit_value = index.data(role=QtCore.Qt.EditRole)
        editor.setValue(edit_value)

    def setModelData(self, editor, model, index):
        if not index.isValid():
            return False
        model.setData(index, editor.value(), role=QtCore.Qt.EditRole)
        return True
