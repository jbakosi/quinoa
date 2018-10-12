# vim: filetype=sh:
# This is a comment
# Keywords are case-sensitive

title "Initial uniform mesh refinement"

inciter

  nstep 10     # Max number of time steps
  cfl   0.8   # CFL coefficient
  ttyi 1      # TTY output interval

  scheme diagcg

  reorder true

  partitioning
    algorithm mj
  end

  amr
    t0ref true
    initial uniform
  end

  transport
    depvar c
    physics advection
    problem slot_cyl
  end

  plotvar
    interval 2
  end

end
