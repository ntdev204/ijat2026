"""
Coverage path planner for full-map coverage using boustrophedon (lawnmower) pattern.
Generates waypoints that cover the entire navigable area of the map.
"""
import math
from typing import List, Dict, Tuple, Optional

import numpy as np
from nav_msgs.msg import OccupancyGrid


class CoveragePlanner:
    """Generate full-map coverage paths using boustrophedon decomposition."""

    def __init__(self, robot_width: float = 0.5, overlap: float = 0.1):
        """
        Initialize coverage planner.

        Args:
            robot_width: Width of robot footprint in meters
            overlap: Overlap ratio between passes (0.0-1.0)
        """
        self.robot_width = robot_width
        self.overlap = overlap
        self.min_clearance = 0.3  # Minimum clearance from obstacles in meters

    def generate_coverage_waypoints(
        self,
        map_msg: OccupancyGrid,
        pattern: str = "boustrophedon"
    ) -> List[Dict[str, float]]:
        """
        Generate coverage waypoints from occupancy grid map.

        Args:
            map_msg: OccupancyGrid message from /map topic
            pattern: Coverage pattern type ("boustrophedon" or "spiral")

        Returns:
            List of waypoint dicts with keys: x, y, yaw
        """
        if pattern == "boustrophedon":
            return self._generate_boustrophedon(map_msg)
        elif pattern == "spiral":
            return self._generate_spiral(map_msg)
        else:
            raise ValueError(f"Unknown pattern type: {pattern}")

    def _generate_boustrophedon(self, map_msg: OccupancyGrid) -> List[Dict[str, float]]:
        """Generate back-and-forth lawnmower pattern."""
        resolution = map_msg.info.resolution
        width = map_msg.info.width
        height = map_msg.info.height
        origin_x = map_msg.info.origin.position.x
        origin_y = map_msg.info.origin.position.y

        # Convert occupancy grid to numpy array
        grid = np.array(map_msg.data).reshape((height, width))

        # Calculate stripe width (accounting for overlap)
        stripe_width_m = self.robot_width * (1.0 - self.overlap)
        stripe_width_cells = max(1, int(stripe_width_m / resolution))

        # Inflate obstacles by robot radius + clearance
        inflation_cells = int((self.robot_width / 2.0 + self.min_clearance) / resolution)
        inflated_grid = self._inflate_obstacles(grid, inflation_cells)

        waypoints = []
        y_cell = 0
        direction = 1  # 1 for left-to-right, -1 for right-to-left

        while y_cell < height:
            # Find free space in this row
            row = inflated_grid[y_cell, :]
            free_segments = self._find_free_segments(row)

            if free_segments:
                # Convert to world coordinates
                y_world = origin_y + (y_cell + 0.5) * resolution

                for x_start, x_end in free_segments:
                    if direction == 1:
                        # Left to right
                        x_world_start = origin_x + (x_start + 0.5) * resolution
                        x_world_end = origin_x + (x_end - 0.5) * resolution
                        yaw = 0.0
                    else:
                        # Right to left
                        x_world_start = origin_x + (x_end - 0.5) * resolution
                        x_world_end = origin_x + (x_start + 0.5) * resolution
                        yaw = math.pi

                    # Add start waypoint
                    waypoints.append({
                        "x": float(x_world_start),
                        "y": float(y_world),
                        "yaw": float(yaw)
                    })

                    # Add end waypoint
                    waypoints.append({
                        "x": float(x_world_end),
                        "y": float(y_world),
                        "yaw": float(yaw)
                    })

                # Alternate direction for next row
                direction *= -1

            # Move to next stripe
            y_cell += stripe_width_cells

        # Optimize waypoints (remove redundant intermediate points)
        optimized = self._optimize_waypoints(waypoints)

        return optimized

    def _generate_spiral(self, map_msg: OccupancyGrid) -> List[Dict[str, float]]:
        """Generate spiral pattern from center outward."""
        resolution = map_msg.info.resolution
        width = map_msg.info.width
        height = map_msg.info.height
        origin_x = map_msg.info.origin.position.x
        origin_y = map_msg.info.origin.position.y

        # Start from map center
        center_x = width // 2
        center_y = height // 2

        waypoints = []

        # Spiral outward in rectangular pattern
        layer = 1
        max_layer = max(width, height) // 2

        while layer <= max_layer:
            # Top edge (left to right)
            for x in range(center_x - layer, center_x + layer + 1):
                if 0 <= x < width and 0 <= center_y - layer < height:
                    x_world = origin_x + (x + 0.5) * resolution
                    y_world = origin_y + (center_y - layer + 0.5) * resolution
                    waypoints.append({"x": float(x_world), "y": float(y_world), "yaw": 0.0})

            # Right edge (top to bottom)
            for y in range(center_y - layer, center_y + layer + 1):
                if 0 <= center_x + layer < width and 0 <= y < height:
                    x_world = origin_x + (center_x + layer + 0.5) * resolution
                    y_world = origin_y + (y + 0.5) * resolution
                    waypoints.append({"x": float(x_world), "y": float(y_world), "yaw": math.pi / 2})

            # Bottom edge (right to left)
            for x in range(center_x + layer, center_x - layer - 1, -1):
                if 0 <= x < width and 0 <= center_y + layer < height:
                    x_world = origin_x + (x + 0.5) * resolution
                    y_world = origin_y + (center_y + layer + 0.5) * resolution
                    waypoints.append({"x": float(x_world), "y": float(y_world), "yaw": math.pi})

            # Left edge (bottom to top)
            for y in range(center_y + layer, center_y - layer - 1, -1):
                if 0 <= center_x - layer < width and 0 <= y < height:
                    x_world = origin_x + (center_x - layer + 0.5) * resolution
                    y_world = origin_y + (y + 0.5) * resolution
                    waypoints.append({"x": float(x_world), "y": float(y_world), "yaw": -math.pi / 2})

            layer += 1

        return waypoints

    def _inflate_obstacles(self, grid: np.ndarray, inflation_cells: int) -> np.ndarray:
        """Inflate obstacles by specified number of cells."""
        from scipy.ndimage import binary_dilation

        # Treat unknown (-1) and occupied (>50) as obstacles
        obstacles = (grid == -1) | (grid > 50)

        # Dilate obstacles
        if inflation_cells > 0:
            structure = np.ones((2 * inflation_cells + 1, 2 * inflation_cells + 1))
            inflated = binary_dilation(obstacles, structure=structure)
        else:
            inflated = obstacles

        return inflated.astype(np.uint8) * 100

    def _find_free_segments(self, row: np.ndarray, min_length: int = 3) -> List[Tuple[int, int]]:
        """Find continuous free space segments in a row."""
        segments = []
        in_segment = False
        start = 0

        for i, cell in enumerate(row):
            if cell == 0:  # Free space
                if not in_segment:
                    start = i
                    in_segment = True
            else:  # Obstacle or unknown
                if in_segment:
                    if i - start >= min_length:
                        segments.append((start, i))
                    in_segment = False

        # Handle segment at end of row
        if in_segment and len(row) - start >= min_length:
            segments.append((start, len(row)))

        return segments

    def _optimize_waypoints(self, waypoints: List[Dict[str, float]]) -> List[Dict[str, float]]:
        """Remove redundant waypoints on straight lines."""
        if len(waypoints) < 3:
            return waypoints

        optimized = [waypoints[0]]

        for i in range(1, len(waypoints) - 1):
            prev = waypoints[i - 1]
            curr = waypoints[i]
            next_wp = waypoints[i + 1]

            # Check if current point is on line between prev and next
            dx1 = curr["x"] - prev["x"]
            dy1 = curr["y"] - prev["y"]
            dx2 = next_wp["x"] - curr["x"]
            dy2 = next_wp["y"] - curr["y"]

            # Cross product to check collinearity
            cross = dx1 * dy2 - dy1 * dx2

            # Keep point if not collinear (threshold for numerical stability)
            if abs(cross) > 0.01:
                optimized.append(curr)

        optimized.append(waypoints[-1])

        return optimized

    def validate_waypoints(
        self,
        waypoints: List[Dict[str, float]],
        map_msg: OccupancyGrid
    ) -> Tuple[bool, Optional[str]]:
        """
        Validate generated waypoints.

        Returns:
            (is_valid, error_message)
        """
        if len(waypoints) < 2:
            return False, "Coverage route must have at least 2 waypoints"

        if len(waypoints) > 1000:
            return False, f"Coverage route has too many waypoints ({len(waypoints)}), max 1000"

        # Check all waypoints are within map bounds
        resolution = map_msg.info.resolution
        width = map_msg.info.width
        height = map_msg.info.height
        origin_x = map_msg.info.origin.position.x
        origin_y = map_msg.info.origin.position.y

        for i, wp in enumerate(waypoints):
            x_cell = int((wp["x"] - origin_x) / resolution)
            y_cell = int((wp["y"] - origin_y) / resolution)

            if not (0 <= x_cell < width and 0 <= y_cell < height):
                return False, f"Waypoint {i} is outside map bounds"

            # Check if waypoint is in free space
            grid_index = y_cell * width + x_cell
            if grid_index < len(map_msg.data):
                cell_value = map_msg.data[grid_index]
                if cell_value > 50 or cell_value == -1:
                    return False, f"Waypoint {i} is in occupied or unknown space"

        return True, None
