#pragma once

namespace editor {

class EditSnapshotRegistry;

namespace generated {

bool initializeSnapshotPolicies(EditSnapshotRegistry& snapshots);

}

} // namespace editor