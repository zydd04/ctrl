#include "Detector.hpp"

#include <algorithm>

Detector::Detector(int holdFrames) : holdFrames(std::max(1, holdFrames)) {}

int Detector::ToMask(const Fingers& fingers) {
	int mask = 0;
	for (int f = 0; f < 5; ++f) {
		if (fingers[f]) {
			mask |= 1 << f;
		}
	}
	return mask;
}

bool Detector::BindCount(int count, Key key) {
	if (count < 0 || count > 5 || key == 0) {
		return false;
	}
	countToKey[count] = key;
	return true;
}

bool Detector::BindFingers(const Fingers& fingers, Key key) {
	if (key == 0) {
		return false;
	}
	fingersToKey[ToMask(fingers)] = key;
	return true;
}

void Detector::UnbindCount(int count) {
	countToKey.erase(count);
}

void Detector::UnbindFingers(const Fingers& fingers) {
	fingersToKey.erase(ToMask(fingers));
}

void Detector::ClearBindings() {
	countToKey.clear();
	fingersToKey.clear();
}

Key Detector::KeyFor(const HandInfo& hand) const {
	if (!hand.found) {
		return 0;
	}
	// Exact finger combination first, then the plain count
	if (auto it = fingersToKey.find(ToMask(hand.fingerUp)); it != fingersToKey.end()) {
		return it->second;
	}
	if (auto it = countToKey.find(hand.fingerCount); it != countToKey.end()) {
		return it->second;
	}
	return 0;
}

Key Detector::Detect(const HandInfo& hand) {
	int gesture = hand.found ? ToMask(hand.fingerUp) : -1;
	if (gesture != currentGesture) {
		currentGesture = gesture;
		heldFrames = 0;
	}
	// Fire only on the frame the hold time is reached, not on every frame after
	if (++heldFrames != holdFrames) {
		return 0;
	}
	return KeyFor(hand);
}
