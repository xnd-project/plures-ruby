class String
  # Force encode string to US-ASCII.
  #
  # FIXME: Hopefully find a better way of doing this soon.
  def b
    self.force_encoding "US-ASCII"
  end
end
