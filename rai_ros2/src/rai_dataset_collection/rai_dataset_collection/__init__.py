"""rai_dataset_collection package for CA-NMPC dataset collection."""

__version__ = '1.0.0'
__author__ = 'Nguyen Ngoc Thien'

# Import main modules for easier access
from .dataset_collector_node import DatasetCollectorNode
from .trajectory_executor import TrajectoryExecutor
from .context_monitor import ContextMonitor