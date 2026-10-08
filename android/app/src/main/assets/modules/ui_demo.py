"""UI demo: modules describe panels, they never draw. Replace/edit and re-import (no restart)."""
import dream

p = dream.props
grav = p.register("world.gravity", float, 9.8, 0, 30, category="Physics", tags=["debug"])
fps = p.register("stats.fps", float, 0, 0, 1000, category="Stats")

dream.ui.panel("World").label("Physics settings").query(category="Physics")
dream.ui.panel("Debug").label("tagged 'debug' only").query(tag="debug").value(fps, "FPS")
# The "Render" panel (fov / wireframe / spin / distance) is declared by the render_cube module itself.


def on_update(dt):
    if dt > 0:
        p.set(fps, 1.0 / dt)   # handle-based: no string lookups per frame
