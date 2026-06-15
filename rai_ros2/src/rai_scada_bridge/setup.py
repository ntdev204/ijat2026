from setuptools import find_packages, setup

package_name = 'rai_scada_bridge'

setup(
    name=package_name,
    version='1.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages', ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        ('share/' + package_name + '/launch', ['launch/scada_bridge.launch.py']),
    ],
    install_requires=['setuptools', 'pyzmq', 'opencv-python-headless', 'numpy', 'scipy'],
    zip_safe=True,
    maintainer='thientn204',
    maintainer_email='thientn204@gmail.com',
    description='ZMQ-ROS2 bridge node for SCADA web interface',
    license='MIT',
    entry_points={
        'console_scripts': [
            'scada_bridge = rai_scada_bridge.main:main',
        ],
    },
)
