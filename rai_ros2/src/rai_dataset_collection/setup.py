from setuptools import setup, find_packages
import os
from glob import glob

package_name = 'rai_dataset_collection'

setup(
    name=package_name,
    version='1.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        (os.path.join('share', package_name, 'launch'),
            glob('launch/*.launch.py')),
        (os.path.join('share', package_name, 'scripts'),
            glob('scripts/*.sh') + glob('scripts/*.py')),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='Nguyen Ngoc Thien',
    maintainer_email='thientn204@utc.edu.vn',
    description='CA-NMPC Dataset Collection System',
    license='MIT',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'dataset_collector = rai_dataset_collection.dataset_collector_node:main',
            'context_monitor = rai_dataset_collection.context_monitor:main',
            'trajectory_executor = rai_dataset_collection.trajectory_executor:main',
        ],
    },
)
