
# -----------------------------------------------------------------------------
# This source file has been developed within the scope of the
# Technical Director course at Filmakademie Baden-Wuerttemberg.
# http://technicaldirector.de
#
# Written by Tim Lehr
# Copyright (c) 2019 Animationsinstitut of Filmakademie Baden-Wuerttemberg
# -----------------------------------------------------------------------------

import logging
import logging.handlers
import os

_default_loglevel = logging.DEBUG
_user_loglevel = None
_override_loglevel = None


def _update_user_loglevel():
    global _user_loglevel
    if not _user_loglevel:
        level_str = os.environ.get("HYPERDRIVE_PYTHON_LOGLEVEL", None)
        _user_loglevel = level_str


def get_log_level():
    return _override_loglevel or _user_loglevel or _default_loglevel


class NoExceptionFormatter(logging.Formatter):
    """Formatter implementation that hides non-critical (caught) exceptions.
    """

    def format(self, record):
        if hasattr(record, 'levelname') and record.levelname != "CRITICAL":
            record.exc_text = ''  # ensure formatException gets called
        return super(NoExceptionFormatter, self).format(record)

    def formatException(self, record):
        if hasattr(record, 'levelname') and record.levelname != "CRITICAL":
            return ''
        return super(NoExceptionFormatter, self).formatException(record)


def get_logger(logger_name):
    """Returns a logger instance with name ``logger_name``

    Returns:
        obj:`logging`: logger instance
    """
    _update_user_loglevel()

    # create logger for module 'name'
    logger = logging.getLogger(logger_name)

    if len(logger.handlers) >= 2:
        return logger

    # prevent logger from propagating messages to other levels
    logger.propagate = False

    # default logging level
    logger.setLevel(get_log_level())

    # create / configure stream handler
    stream_handler = logging.StreamHandler()
    stream_format = NoExceptionFormatter("%(levelname)-5.5s | %(name)s | %(message)s")
    stream_handler.setLevel(get_log_level())
    stream_handler.setFormatter(stream_format)

    # add handlers
    logger.addHandler(stream_handler)

    return logger


def set_log_level(level):
    global _loglevel_stream_override
    log = get_logger(__name__)
    log.info("Set logging level for Python loggers to: '{}'".format(level))
    loggers = [logging.getLogger()] + logging.Logger.manager.loggerDict.values()

    for logger in [x for x in loggers if isinstance(x, logging.Filterer)]:
        for handler in logger.handlers:
                handler.setLevel(level)
    _loglevel_stream_override = level