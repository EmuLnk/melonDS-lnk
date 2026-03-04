package me.magnum.melonds.ui.settings.fragments

import android.os.Bundle
import com.smp.masterswitchpreference.MasterSwitchPreferenceFragment
import me.magnum.melonds.R
import me.magnum.melonds.ui.settings.PreferenceFragmentTitleProvider

class EmuLnkPreferencesFragment : MasterSwitchPreferenceFragment(), PreferenceFragmentTitleProvider {

    override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
        super.onCreatePreferences(savedInstanceState, rootKey)
    }

    override fun getTitle() = getString(R.string.emulnk)
}
