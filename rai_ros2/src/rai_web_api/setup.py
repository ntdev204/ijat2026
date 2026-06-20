from glob import glob
import os

from setuptools import find_packages, setup

package_name = 'rai_web_api'

setup(
    name=package_name,
    version='1.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages', ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        (os.path.join('share', package_name, 'launch'), glob('launch/*.launch.py')),
        (os.path.join('share', package_name, 'frontend'), glob('frontend/*')),
    ],
    install_requires=[
        'setuptools',
        'fastapi>=0.100.0',
        'uvicorn[standard]>=0.20.0',
        'SQLAlchemy>=2.0.0',
        'asyncpg>=0.27.0',
        'aiosqlite>=0.19.0',
        'pydantic>=2.0.0',
        'aiortc>=1.6.0',
        'av>=10.0.0',
        'numpy>=1.21.0',
        'pyzmq>=25.0.0',
    ],
    zip_safe=True,
    maintainer='Nguyen Ngoc Thien',
    maintainer_email='thientn204@utc.edu.vn',
    description='FastAPI Web API for Rai Robot Realtime Monitoring and Control',
    license='MIT',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'web_api = rai_web_api.main:main',
        ],
    },
)
