#pragma once

#include <array>
#include <unordered_map>
#include "FingerDetector.hpp"
#include "Input.hpp"

// Translates the fingers found by FingerDetector into keys, using bindings
// chosen by the user. Bindings can be changed at any time (e.g. from the UI).
//
// Two kinds of binding:
//   - by count:   "3 fingers raised"  -> key   (any 3 fingers)
//   - by fingers: "index + middle up" -> key   (exactly those fingers)
// When both match, the exact finger binding wins.

// Which fingers are raised: thumb, index, middle, ring, pinky
using Fingers = std::array<bool, 5>;

class Detector {
public:
	// holdFrames: how many consecutive frames a gesture must be seen before its
	// key is sent, so a flickering detection doesn't cause stray presses
	explicit Detector(int holdFrames = 5);

	// count is 0..5 (0 = fist). Returns false if count is out of range or key is 0.
	bool BindCount(int count, Key key);
	bool BindFingers(const Fingers& fingers, Key key);

	void UnbindCount(int count);
	void UnbindFingers(const Fingers& fingers);
	void ClearBindings();

	// Key bound to this hand right now, ignoring the hold time (0 if none)
	Key KeyFor(const HandInfo& hand) const;

	// Call once per frame. Returns the bound key once when a gesture has been
	// held for holdFrames, then 0 until the gesture changes (or the hand leaves
	// and comes back), so holding a gesture presses its key a single time.
	Key Detect(const HandInfo& hand);

private:
	static int ToMask(const Fingers& fingers); // one bit per finger

	std::unordered_map<int, Key> countToKey;
	std::unordered_map<int, Key> fingersToKey; // keyed by finger mask

	int holdFrames;
	int currentGesture = -1; // finger mask seen in the last frame, -1 = no hand
	int heldFrames = 0;      // consecutive frames currentGesture has been seen
};
