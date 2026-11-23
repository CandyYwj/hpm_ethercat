from setuptools import find_packages, setup

package_name = "hpm_gateway_power"

setup(
    name=package_name,
    version="0.1.0",
    packages=find_packages(exclude=["test"]),
    data_files=[
        ("share/ament_index/resource_index/packages", ["resource/" + package_name]),
        ("share/" + package_name, ["package.xml"]),
        ("share/" + package_name + "/launch", ["launch/power_node.launch.py"]),
        ("share/" + package_name + "/config", ["config/power_node.yaml"]),
    ],
    install_requires=["setuptools", "pyserial"],
    zip_safe=True,
    maintainer="hpm_gateway",
    maintainer_email="hpm_gateway@example.com",
    description="PowerLink v1 power board control node for the DOG Power Board D0.",
    license="Apache-2.0",
    tests_require=["pytest"],
    entry_points={
        "console_scripts": [
            "power_node = hpm_gateway_power.power_node:main",
            "power_board_cli = hpm_gateway_power.power_board_cli:main",
        ],
    },
)
