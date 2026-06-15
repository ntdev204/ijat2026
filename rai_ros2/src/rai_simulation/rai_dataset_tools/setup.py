from setuptools import setup
import os

package_name = 'rai_dataset_tools'

setup(
    name=package_name,
    version='1.0.0',
    packages=[],
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='User',
    maintainer_email='thientn204@todo.todo',
    description='Dataset processing and extraction tools for CCA-NMPC',
    license='Apache-2.0',
    scripts=[
        'bag_to_parquet.py',
        'validate_bag.py',
        'sync_topics.py',
        'build_run_index.py',
        'dataset_split.py',
        'process_dataset.py'
    ]
)
