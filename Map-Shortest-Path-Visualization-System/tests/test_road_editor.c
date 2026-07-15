#include "road_editor.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

int main(void) {
    RoadEditor editor;
    MapPoint point;
    MapTransform transform = {2.0f, 3.0f, -20.0f, -60.0f, 0};
    Vector3 world = map_to_world(15.0f, 30.0f, &transform);

    assert(world_to_map(world, &transform, &point));
    assert(fabs(point.x - 15.0) < 1e-6);
    assert(fabs(point.y - 30.0) < 1e-6);

    road_editor_init(&editor);
    road_editor_toggle(&editor);
    road_editor_update_cursor(&editor, (MapPoint){100.0, 100.0}, 1);
    road_editor_add_node(&editor);
    road_editor_update_cursor(&editor, (MapPoint){200.0, 100.0}, 1);
    road_editor_add_node(&editor);
    assert(editor.node_count == 2);

    road_editor_update_cursor(&editor, (MapPoint){100.0, 100.0}, 1);
    road_editor_add_control_point(&editor);
    road_editor_update_cursor(&editor, (MapPoint){150.0, 110.0}, 1);
    road_editor_add_control_point(&editor);
    road_editor_update_cursor(&editor, (MapPoint){200.0, 100.0}, 1);
    assert(road_editor_finish_edge(&editor));
    assert(editor.edge_count == 1);
    assert(editor.edges[0].from_id == 1);
    assert(editor.edges[0].to_id == 2);
    assert(editor.edges[0].point_count == 3);
    assert(editor.edges[0].weight > 100.0);

    puts("road editor tests passed");
    return 0;
}
